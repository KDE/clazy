/*
   This file is part of the clazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "checkmanager.h"
#include "clazy_stl.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "StringUtils.h"

#include <stdlib.h>

using namespace clang;
using namespace std;

static const char * s_fixitNamePrefix = "fix-";
static const char * s_levelPrefix = "level";

std::mutex CheckManager::m_lock;

CheckManager::CheckManager()
{
    m_registeredChecks.reserve(100);
}

bool CheckManager::checkExists(const string &name) const
{
    return checkForName(m_registeredChecks, name) != m_registeredChecks.cend();
}

bool CheckManager::isReservedCheckName(const string &name) const
{
    static const vector<string> names = { "clazy" };
    if (clazy_std::contains(names, name))
        return true;

    // level0, level1, etc are not allowed
    if (clazy_std::startsWith(name, s_levelPrefix))
        return true;

    // These are fixit names
    if (clazy_std::startsWith(name, s_fixitNamePrefix))
        return true;

    return false;
}

CheckManager *CheckManager::instance()
{
    static CheckManager s_instance;
    return &s_instance;
}

int CheckManager::registerCheck(const std::string &name, const string &className,
                                CheckLevel level, const FactoryFunction &factory,
                                RegisteredCheck::Options options)
{
    assert(factory != nullptr);
    assert(!name.empty());

    if (isReservedCheckName(name)) {
        llvm::errs() << "Check name not allowed" << name;
        assert(false);
    } else {
        m_registeredChecks.push_back({name, className, level, factory, options});
    }

    return 0;
}

int CheckManager::registerFixIt(int id, const string &fixitName, const string &checkName)
{
    if (fixitName.empty() || checkName.empty()) {
        assert(false);
        return 0;
    }

    if (!clazy_std::startsWith(fixitName, s_fixitNamePrefix)) {
        assert(false);
        return 0;
    }

    auto &fixits = m_fixitsByCheckName[checkName];
    for (const auto& fixit : fixits) {
        if (fixit.name == fixitName) {
            // It can't exist
            assert(false);
            return 0;
        }
    }
    RegisteredFixIt fixit = {id, fixitName};
    fixits.push_back(fixit);
    m_fixitByName.insert({fixitName, fixit});

    return 0;
}

CheckBase* CheckManager::createCheck(const string &name, ClazyContext *context)
{
    for (const auto& rc : m_registeredChecks) {
        if (rc.name == name) {
            return rc.factory(context);
        }
    }

    llvm::errs() << "Invalid check name " << name << "\n";
    return nullptr;
}

string CheckManager::checkNameForFixIt(const string &fixitName) const
{
    if (fixitName.empty())
        return {};

    for (auto &registeredCheck : m_registeredChecks) {
        auto it = m_fixitsByCheckName.find(registeredCheck.name);
        if (it != m_fixitsByCheckName.end()) {
            auto &fixits = (*it).second;
            for (const RegisteredFixIt &fixit : fixits) {
                if (fixit.name == fixitName)
                    return (*it).first;
            }
        }
    }

    return {};
}

RegisteredCheck::List CheckManager::availableChecks(CheckLevel maxLevel) const
{
    RegisteredCheck::List checks = m_registeredChecks;

    checks.erase(remove_if(checks.begin(), checks.end(),
                           [maxLevel](const RegisteredCheck &r) { return r.level > maxLevel; }), checks.end());


    return checks;
}

RegisteredCheck::List CheckManager::requestedChecksThroughEnv(const ClazyContext *context) const
{
    vector<string> dummy;
    return requestedChecksThroughEnv(context, dummy);
}

RegisteredCheck::List CheckManager::requestedChecksThroughEnv(const ClazyContext *context, vector<string> &userDisabledChecks) const
{
    static RegisteredCheck::List requestedChecksThroughEnv;
    if (requestedChecksThroughEnv.empty()) {
        const char *checksEnv = getenv("CLAZY_CHECKS");
        if (checksEnv) {
            const string checksEnvStr = clazy_std::unquoteString(checksEnv);
            requestedChecksThroughEnv = checksEnvStr == "all_checks" ? availableChecks(CheckLevel2)
                                                                     : checksForCommaSeparatedString(checksEnvStr, /*by-ref=*/ userDisabledChecks);
        }

        const string checkName = checkNameForFixIt(context->requestedFixitName);
        if (!checkName.empty() && checkForName(requestedChecksThroughEnv, checkName) == requestedChecksThroughEnv.cend()) {
            requestedChecksThroughEnv.push_back(*checkForName(m_registeredChecks, checkName));
        }
    }

    return requestedChecksThroughEnv;
}

RegisteredCheck::List::const_iterator CheckManager::checkForName(const RegisteredCheck::List &checks,
                                                                 const string &name) const
{
    return clazy_std::find_if(checks, [name](const RegisteredCheck &r) {
        return r.name == name;
    } );
}

RegisteredFixIt::List CheckManager::availableFixIts(const string &checkName) const
{
    auto it = m_fixitsByCheckName.find(checkName);
    return it == m_fixitsByCheckName.end() ? RegisteredFixIt::List() : (*it).second;
}

static bool takeArgument(const string &arg, vector<string> &args)
{
    auto it = clazy_std::find(args, arg);
    if (it != args.end()) {
        args.erase(it, it + 1);
        return true;
    }

    return false;
}

RegisteredCheck::List CheckManager::requestedChecks(const ClazyContext *context, std::vector<std::string> &args)
{
    RegisteredCheck::List result;

    // #1 Check if a level was specified
    static const vector<string> levels = { "level0", "level1", "level2", "level3", "level4" };
    const int numLevels = levels.size();
    CheckLevel requestedLevel = CheckLevelUndefined;
    for (int i = 0; i < numLevels; ++i) {
        if (takeArgument(levels.at(i), args)) {
            requestedLevel = static_cast<CheckLevel>(i);
            break;
        }
    }

    if (args.size() > 1) // we only expect a level and a comma separated list of arguments
        return {};

    if (args.size() == 1) {
        // #2 Process list of comma separated checks that were passed to compiler
        result = checksForCommaSeparatedString(args[0]);
        if (result.empty()) // User passed inexisting checks.
            return {};
    }

    // #3 Append checks specified from env variable

    vector<string> userDisabledChecks;
    RegisteredCheck::List checksFromEnv = requestedChecksThroughEnv(context, /*by-ref*/userDisabledChecks);
    copy(checksFromEnv.cbegin(), checksFromEnv.cend(), back_inserter(result));

    if (result.empty() && requestedLevel == CheckLevelUndefined) {
        // No checks or level specified, lets use the default level
        requestedLevel = DefaultCheckLevel;
    }

    // #4 Add checks from requested level
    RegisteredCheck::List checksFromRequestedLevel = checksForLevel(requestedLevel);
    clazy_std::append(checksFromRequestedLevel, result);
    clazy_std::sort_and_remove_dups(result, checkLessThan);
    CheckManager::removeChecksFromList(result, userDisabledChecks);

    if (context->options & ClazyContext::ClazyOption_Qt4Compat) {
        // #5 Remove Qt4 incompatible checks
        result.erase(remove_if(result.begin(), result.end(), [](const RegisteredCheck &c){
           return c.options & RegisteredCheck::Option_Qt4Incompatible;
        }), result.end());
    }

    return result;
}

RegisteredCheck::List CheckManager::checksForLevel(int level) const
{
    RegisteredCheck::List result;
    if (level > CheckLevelUndefined && level <= MaxCheckLevel) {
        clazy_std::append_if(m_registeredChecks, result, [level](const RegisteredCheck &r) {
            return r.level <= level;
        });
    }

    return result;
}

CheckBase::List CheckManager::createChecks(const RegisteredCheck::List &requestedChecks,
                                           ClazyContext *context)
{
    assert(context);
    const string fixitCheckName = checkNameForFixIt(context->requestedFixitName);
    RegisteredFixIt fixit = m_fixitByName[context->requestedFixitName];

    CheckBase::List checks;
    checks.reserve(requestedChecks.size() + 1);
    for (const auto& check : requestedChecks) {
        checks.push_back(createCheck(check.name, context));
        if (check.name == fixitCheckName) {
            checks.back()->setEnabledFixits(fixit.id);
        }
    }

    if (!context->requestedFixitName.empty()) {
        // We have one fixit enabled, we better have the check instance too.
        if (!fixitCheckName.empty()) {
            if (checkForName(requestedChecks, fixitCheckName) == requestedChecks.cend()) {
                checks.push_back(createCheck(fixitCheckName, context));
                checks.back()->setEnabledFixits(fixit.id);
            }
        }
    }

    return checks;
}

/*static */
void CheckManager::removeChecksFromList(RegisteredCheck::List &list, vector<string> &checkNames)
{
    for (auto &name : checkNames) {
        list.erase(remove_if(list.begin(), list.end(), [name](const RegisteredCheck &c) {
            return c.name == name;
        }), list.end());
    }
}

RegisteredCheck::List CheckManager::checksForCommaSeparatedString(const string &str) const
{
    vector<string> byRefDummy;
    return checksForCommaSeparatedString(str, byRefDummy);
}

RegisteredCheck::List CheckManager::checksForCommaSeparatedString(const string &str,
                                                                  vector<string> &userDisabledChecks) const
{
    vector<string> checkNames = clazy_std::splitString(str, ',');
    RegisteredCheck::List result;

    for (const string &name : checkNames) {
        if (checkForName(result, name) != result.cend())
            continue; // Already added. Duplicate check specified. continue.

        auto it = checkForName(m_registeredChecks, name);
        if (it == m_registeredChecks.cend()) {
            // Unknown, but might be a fixit name
            const string checkName = checkNameForFixIt(name);
            auto it = checkForName(m_registeredChecks, checkName);
            const bool checkDoesntExist = it == m_registeredChecks.cend();
            if (checkDoesntExist) {
                if (clazy_std::startsWith(name, s_levelPrefix) && name.size() == strlen(s_levelPrefix) + 1) {
                    auto lastChar = name.back();
                    const int digit = lastChar - '0';
                    if (digit > CheckLevelUndefined && digit <= MaxCheckLevel) {
                        RegisteredCheck::List levelChecks = checksForLevel(digit);
                        clazy_std::append(levelChecks, result);
                    } else {
                        llvm::errs() << "Invalid level: " << name << "\n";
                    }
                } else {
                    if (clazy_std::startsWith(name, "no-")) {
                        string checkName = name;
                        checkName.erase(0, 3);
                        if (checkExists(checkName)) {
                            userDisabledChecks.push_back(checkName);
                        } else {
                            llvm::errs() << "Invalid check to disable: " << name << "\n";
                        }
                    } else {
                        llvm::errs() << "Invalid check: " << name << "\n";
                    }
                }
            } else {
                result.push_back(*it);
            }
            continue;
        } else {
            result.push_back(*it);
        }
    }

    removeChecksFromList(result, userDisabledChecks);

    return result;
}
