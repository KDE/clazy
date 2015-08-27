/*
   This file is part of the clang-lazy static checker.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  Copyright (C) 2015 Sergio Martins <smartins@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "checkmanager.h"
#include "Utils.h"

#include <stdlib.h>

using namespace std;

struct RegisteredCheck {
    std::string name;
    int flags;
    FactoryFunction factory;
};

CheckManager *CheckManager::instance()
{
    static CheckManager *s_instance = new CheckManager();
    return s_instance;
}

CheckManager::CheckManager() : m_enableAllFixits(false)
{
    m_registeredChecks.reserve(30);
    const char *fixitsEnv = getenv("CLAZY_FIXIT");
    if (fixitsEnv != nullptr) {
        if (string(fixitsEnv) == string("all_fixits"))
            m_enableAllFixits = true;
        else
            m_requestedFixitName = string(fixitsEnv);
    }
}

int CheckManager::registerCheck(const std::string &name, int checkFlags, FactoryFunction factory)
{
    assert(factory != nullptr);
    assert(!name.empty());
    m_registeredChecks.push_back({name, checkFlags, factory});

    return 0;
}

int CheckManager::registerFixIt(int id, const string &fixitName, const string &checkName)
{
    if (fixitName.empty() || checkName.empty()) {
        assert(false);
        return 0;
    }

    auto &fixits = m_fixitsByCheckName[checkName];
    for (auto fixit : fixits) {
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

void CheckManager::setCompilerInstance(clang::CompilerInstance *ci)
{
    m_ci = ci;
    m_sm = &ci->getSourceManager();
}

unique_ptr<CheckBase> CheckManager::createCheck(const string &name)
{
    for (auto rc : m_registeredChecks) {
        if (rc.name == name) {
            return unique_ptr<CheckBase>(rc.factory());
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

vector<string> CheckManager::availableCheckNames(bool includeHidden) const
{
    vector<string> names;
    names.reserve(m_registeredChecks.size());
    for (auto rc : m_registeredChecks) {
        if (includeHidden || !(rc.flags & HiddenFlag))
            names.push_back(rc.name);
    }
    return names;
}

std::vector<string> CheckManager::requestedCheckNamesThroughEnv() const
{
    static vector<string> requestedChecksThroughEnv;
    if (requestedChecksThroughEnv.empty()) {
        const char *checksEnv = getenv("CLAZY_CHECKS");
        if (checksEnv != nullptr) {
            requestedChecksThroughEnv = checkNamesForCommaSeparatedString(checksEnv);
        }
        string checkName = checkNameForFixIt(m_requestedFixitName);
        if (!checkName.empty() && find(requestedChecksThroughEnv.cbegin(), requestedChecksThroughEnv.cend(), checkName) == requestedChecksThroughEnv.cend())
            requestedChecksThroughEnv.push_back(checkName);
    }

    return requestedChecksThroughEnv;
}

RegisteredFixIt::List CheckManager::availableFixIts(const string &checkName) const
{
    auto it = m_fixitsByCheckName.find(checkName);
    return it == m_fixitsByCheckName.end() ? RegisteredFixIt::List() : (*it).second;
}

void CheckManager::createChecks(vector<string> requestedChecks)
{
    const string fixitCheckName = checkNameForFixIt(m_requestedFixitName);
    RegisteredFixIt fixit = m_fixitByName[m_requestedFixitName];

    m_createdChecks.clear();
    m_createdChecks.reserve(requestedChecks.size() + 1);
    for (auto checkName : requestedChecks) {
        m_createdChecks.push_back(createCheck(checkName));
        if (checkName == fixitCheckName) {
            m_createdChecks.back()->setEnabledFixits(fixit.id);
        }
    }

    if (!m_requestedFixitName.empty()) {
        // We have one fixit enabled, we better have the check instance too.
        if (!fixitCheckName.empty()) {
            auto it = std::find(requestedChecks.cbegin(), requestedChecks.cend(), fixitCheckName);
            if (it == requestedChecks.cend()) {
                m_createdChecks.push_back(createCheck(fixitCheckName));
                m_createdChecks.back()->setEnabledFixits(fixit.id);
            }
        }

    }
}

const CheckBase::List &CheckManager::createdChecks() const
{
    return m_createdChecks;
}

bool CheckManager::fixitsEnabled() const
{
    return !m_requestedFixitName.empty() || m_enableAllFixits;
}

bool CheckManager::allFixitsEnabled() const
{
    return m_enableAllFixits;
}

std::vector<string> CheckManager::checkNamesForCommaSeparatedString(const string &str) const
{
    vector<string> checkNames = Utils::splitString(str, ',');
    vector<string> result;

    for (const string &name : checkNames) {
        if (find(result.cbegin(), result.cend(), name) != result.cend())
            continue;

        if (find_if(m_registeredChecks.cbegin(), m_registeredChecks.cend(), [&name](const RegisteredCheck &r) { return r.name == name;}) == m_registeredChecks.cend())
            llvm::errs() << "Invalid check: " << name << "\n";
        else
            result.push_back(name);
    }

    return result;
}
