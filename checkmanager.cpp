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

#include "clazy_stl.h"
#include "checkmanager.h"
#include "Utils.h"
#include "StringUtils.h"

#include <stdlib.h>

using namespace clang;
using namespace std;

static const char * s_fixitNamePrefix = "fix-";
static const char * s_levelPrefix = "level";

CheckManager *CheckManager::instance()
{
    static CheckManager s_instance;
    return &s_instance;
}

CheckManager::CheckManager()
    : m_enableAllFixits(false)
    , m_requestedLevel(CheckLevelUndefined)
    , m_extraOptions(clazy_std::splitString(getenv("CLAZY_EXTRA_OPTIONS"), ','))
{
    m_registeredChecks.reserve(30);
    const char *fixitsEnv = getenv("CLAZY_FIXIT");
    if (fixitsEnv) {
        if (string(fixitsEnv) == string("all_fixits")) {
            m_enableAllFixits = true;
        } else {
            m_requestedFixitName = string(fixitsEnv);
        }
    }
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

int CheckManager::registerCheck(const std::string &name, CheckLevel level, FactoryFunction factory)
{
    assert(factory != nullptr);
    assert(!name.empty());

    if (isReservedCheckName(name)) {
        llvm::errs() << "Check name not allowed" << name;
        assert(false);
    } else {
        m_registeredChecks.push_back({name, level, factory});
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

unique_ptr<CheckBase> CheckManager::createCheck(const string &name, const CompilerInstance &ci)
{
    for (const auto& rc : m_registeredChecks) {
        if (rc.name == name) {
            return unique_ptr<CheckBase>(rc.factory(ci));
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

RegisteredCheck::List CheckManager::requestedChecksThroughEnv() const
{
    static RegisteredCheck::List requestedChecksThroughEnv;
    if (requestedChecksThroughEnv.empty()) {
        const char *checksEnv = getenv("CLAZY_CHECKS");
        if (checksEnv) {
            requestedChecksThroughEnv = string(checksEnv) == "all_checks" ? availableChecks(CheckLevel2)
                                                                          : checksForCommaSeparatedString(checksEnv);
        }

        const string checkName = checkNameForFixIt(m_requestedFixitName);
        if (!checkName.empty() && checkForName(requestedChecksThroughEnv, checkName) == requestedChecksThroughEnv.cend()) {
            requestedChecksThroughEnv.push_back(*checkForName(m_registeredChecks, checkName));
        }
    }

    return requestedChecksThroughEnv;
}

RegisteredCheck::List::const_iterator CheckManager::checkForName(const RegisteredCheck::List &checks,
                                                                 const string &name) const
{
    return clazy_std::find_if(checks, [name](RegisteredCheck r) {
        return r.name == name;
    } );
}

RegisteredFixIt::List CheckManager::availableFixIts(const string &checkName) const
{
    auto it = m_fixitsByCheckName.find(checkName);
    return it == m_fixitsByCheckName.end() ? RegisteredFixIt::List() : (*it).second;
}

RegisteredCheck::List CheckManager::checksFromRequestedLevel() const
{
   return checksForLevel(m_requestedLevel);
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

CheckBase::List CheckManager::createChecks(RegisteredCheck::List requestedChecks,
                                           const CompilerInstance &ci)
{
    const string fixitCheckName = checkNameForFixIt(m_requestedFixitName);
    RegisteredFixIt fixit = m_fixitByName[m_requestedFixitName];

    CheckBase::List checks;
    checks.reserve(requestedChecks.size() + 1);
    for (const auto& check : requestedChecks) {
        checks.push_back(createCheck(check.name, ci));
        if (check.name == fixitCheckName) {
            checks.back()->setEnabledFixits(fixit.id);
        }
    }

    if (!m_requestedFixitName.empty()) {
        // We have one fixit enabled, we better have the check instance too.
        if (!fixitCheckName.empty()) {
            if (checkForName(requestedChecks, fixitCheckName) == requestedChecks.cend()) {
                checks.push_back(createCheck(fixitCheckName, ci));
                checks.back()->setEnabledFixits(fixit.id);
            }
        }
    }

    return checks;
}

bool CheckManager::fixitsEnabled() const
{
    return !m_requestedFixitName.empty() || m_enableAllFixits;
}

void CheckManager::enableAllFixIts()
{
    m_enableAllFixits = true;
}

bool CheckManager::allFixitsEnabled() const
{
    return m_enableAllFixits;
}

bool CheckManager::isOptionSet(const string &optionName) const
{
    return clazy_std::contains(m_extraOptions, optionName);
}

RegisteredCheck::List CheckManager::checksForCommaSeparatedString(const string &str) const
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
            if (it == m_registeredChecks.cend()) {
                if (clazy_std::startsWith(name, s_levelPrefix) && name.size() == strlen(s_levelPrefix) + 1) {
                    auto lastChar = name.back();
                    const int digit = lastChar - '0';
                    if (digit > CheckLevelUndefined && digit <= MaxCheckLevel) {
                        RegisteredCheck::List levelChecks = checksForLevel(digit);
                        clazy_std::append(levelChecks, result);
                    } else {
                        llvm::errs() << "Invalid check: " << name << "\n";
                    }
                } else {
                    llvm::errs() << "Invalid check: " << name << "\n";
                }
            } else {
                result.push_back(*it);
            }
            continue;
        } else {
            result.push_back(*it);
        }
    }

    return result;
}

void CheckManager::setRequestedLevel(CheckLevel level)
{
    m_requestedLevel = level;
}

CheckLevel CheckManager::requestedLevel() const
{
    return m_requestedLevel;
}

SuppressionManager *CheckManager::suppressionManager()
{
    return &m_suppressionManager;
}
