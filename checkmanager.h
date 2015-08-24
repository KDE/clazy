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

#ifndef MORE_WARNINGS_CHECK_MANAGER_H
#define MORE_WARNINGS_CHECK_MANAGER_H

#include "checkbase.h"

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>

struct RegisteredCheck;

struct RegisteredFixIt {
    typedef std::vector<RegisteredFixIt> List;
    RegisteredFixIt() : id(-1) {}
    RegisteredFixIt(int id, const std::string &name) : id(id), name(name) {}
    int id = -1;
    std::string name;
    bool operator==(const RegisteredFixIt &other) const { return id == other.id; }
};

using FactoryFunction = std::function<CheckBase*()>;

enum CheckFlag {
    NoFlag = 0,
    HiddenFlag = 1 // Check won't be printed in help, or in "all" mode, but can be used if explicitely specified
};

class CheckManager
{
public:
    static CheckManager *instance();
    int registerCheck(const std::string &name, int checkFlags, FactoryFunction);
    int registerFixIt(int id, const std::string &fititName, const std::string &checkName);

    void setCompilerInstance(clang::CompilerInstance *);
    std::vector<std::string> availableCheckNames(bool includeHidden) const;
    RegisteredFixIt::List availableFixIts(const std::string &checkName) const;
    void createCheckers(const std::vector<std::string> &requestedChecks);
    const CheckBase::List &createdChecks() const;
    bool fixitsEnabled() const;

    // Public for convinence
    clang::CompilerInstance *m_ci;
    clang::SourceManager *m_sm;

    bool allFixitsEnabled() const;

    std::vector<std::string> checkNamesForCommaSeparatedString(const std::string &str) const;

private:
    CheckManager();
    std::unique_ptr<CheckBase> createCheck(const std::string &name);
    std::string checkNameForFixIt(const std::string &) const;
    std::vector<RegisteredCheck> m_registeredChecks;
    CheckBase::List m_createdChecks;
    std::unordered_map<std::string, std::vector<RegisteredFixIt> > m_fixitsByCheckName;
    std::unordered_map<std::string, RegisteredFixIt > m_fixitByName;
    std::string m_requestedFixitName;
    bool m_enableAllFixits;
};

#define REGISTER_CHECK(CHECK_NAME, CLASS_NAME) \
    REGISTER_CHECK_WITH_FLAGS(CHECK_NAME, CLASS_NAME, NoFlag)

#define REGISTER_CHECK_WITH_FLAGS(CHECK_NAME, CLASS_NAME, FLAGS) \
    static int dummy = CheckManager::instance()->registerCheck(CHECK_NAME, (int)FLAGS, [](){ return new CLASS_NAME(CHECK_NAME); }); \
    inline void silence_warning() { (void)dummy; }

#define REGISTER_FIXIT(FIXIT_ID, FIXIT_NAME, CHECK_NAME) \
    static int dummy_##FIXIT_ID = CheckManager::instance()->registerFixIt(FIXIT_ID, FIXIT_NAME, CHECK_NAME); \
    inline void silence_warning_dummy_##FIXIT_ID() { (void)dummy_##FIXIT_ID; }

#endif
