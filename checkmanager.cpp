/**********************************************************************
**  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
**  Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2.1 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**********************************************************************/

#include "checkmanager.h"

#include "stdlib.h"

using namespace std;

struct RegisteredCheck {
    std::string name;
    FactoryFunction factory;
};



CheckManager *CheckManager::instance()
{
    static CheckManager *s_instance = new CheckManager();
    return s_instance;
}

CheckManager::CheckManager()
{
    m_registeredChecks.reserve(30);
    const char *variable = getenv("MORE_WARNINGS_FIXIT");
    if (variable != nullptr) {
        m_requestedFixitName = string(variable);
    }

}

int CheckManager::registerCheck(const std::string &name, FactoryFunction factory)
{
    assert(factory != nullptr);
    assert(!name.empty());
    m_registeredChecks.push_back({ name, factory});

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

    fixits.push_back({id, fixitName});

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

    return nullptr;
}

string CheckManager::checkNameForFixIt(const string &fixitName) const
{
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

vector<string> CheckManager::availableCheckNames() const
{
    vector<string> names;
    names.reserve(m_registeredChecks.size());
    for (auto rc : m_registeredChecks)
        names.push_back(rc.name);
    return names;
}

RegisteredFixIt::List CheckManager::availableFixIts(const string &checkName) const
{
    auto it = m_fixitsByCheckName.find(checkName);
    return it == m_fixitsByCheckName.end() ? RegisteredFixIt::List() : (*it).second;
}

void CheckManager::createCheckers(const vector<string> &requestedChecks)
{
    m_createdChecks.clear();
    m_createdChecks.reserve(requestedChecks.size() + 1);
    for (auto checkName : requestedChecks)
        m_createdChecks.push_back(createCheck(checkName));

    if (!m_requestedFixitName.empty()) {
        // We have one fixit enabled, we better have the check instance too.
        const string checkName = checkNameForFixIt(m_requestedFixitName);
        if (!checkName.empty() && std::find(requestedChecks.cbegin(), requestedChecks.cend(), checkName) == requestedChecks.cend()) {
            m_createdChecks.push_back(createCheck(checkName));
        }
    }
}

const CheckBase::List &CheckManager::createdChecks() const
{
    return m_createdChecks;
}

bool CheckManager::fixitsEnabled() const
{
    return !m_requestedFixitName.empty();
}
