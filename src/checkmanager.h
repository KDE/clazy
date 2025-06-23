/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB a KDAB Group company info@kdab.com
    SPDX-FileContributor: Sérgio Martins <sergio.martins@kdab.com>

    SPDX-FileCopyrightText: 2015-2016 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLANG_LAZY_CHECK_MANAGER_H
#define CLANG_LAZY_CHECK_MANAGER_H

#include "checkbase.h"

#include <clang/Lex/PreprocessorOptions.h>

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class ClazyContext;

struct RegisteredFixIt {
    using List = std::vector<RegisteredFixIt>;
    RegisteredFixIt()
        : id(-1)
    {
    }
    RegisteredFixIt(int id, const std::string &name)
        : id(id)
        , name(name)
    {
    }
    int id = -1;
    std::string name;
    bool operator==(const RegisteredFixIt &other) const
    {
        return id == other.id;
    }
};

using FactoryFunction = std::function<CheckBase *(ClazyContext *context)>;

struct RegisteredCheck {
    enum Option { Option_None = 0, Option_VisitsStmts = 2, Option_VisitsDecls = 4, Option_PreprocessorCallbacks = 8, Option_VisitAllTypeDefs = 16 };

    using List = std::vector<RegisteredCheck>;
    using Options = int;

    std::string name;
    CheckLevel level;
    FactoryFunction factory;
    Options options;
    bool operator==(const RegisteredCheck &other) const
    {
        return name == other.name;
    }
};

inline bool checkLessThan(const RegisteredCheck &c1, const RegisteredCheck &c2)
{
    return c1.name < c2.name;
}

inline bool checkLessThanByLevel(const RegisteredCheck &c1, const RegisteredCheck &c2)
{
    if (c1.level == c2.level) {
        return checkLessThan(c1, c2);
    }

    return c1.level < c2.level;
}

class CheckManager
{
public:
    /**
     * @note You must hold the CheckManager lock when operating on the instance
     *
     * @sa lock()
     */
    static CheckManager *instance();

    static std::mutex &lock()
    {
        return m_lock;
    }
    RegisteredCheck::List availableChecks(CheckLevel maxLevel) const;
    RegisteredCheck::List requestedChecksThroughEnv(std::vector<std::string> &userDisabledChecks) const;
    std::vector<std::string> checksAsErrors() const;

    RegisteredCheck::List::const_iterator checkForName(const RegisteredCheck::List &checks, const std::string &name) const;
    RegisteredCheck::List checksForCommaSeparatedString(const std::string &str) const;
    RegisteredCheck::List checksForCommaSeparatedString(const std::string &str, std::vector<std::string> &userDisabledChecks) const;
    RegisteredFixIt::List availableFixIts(const std::string &checkName) const;

    /**
     * Returns all the requested checks.
     * This is a union of the requested checks via env variable and via arguments passed to compiler
     */
    RegisteredCheck::List requestedChecks(std::vector<std::string> &args);
    std::vector<std::pair<CheckBase *, RegisteredCheck>> createChecks(const RegisteredCheck::List &requestedChecks, ClazyContext *context);

    static void removeChecksFromList(RegisteredCheck::List &list, std::vector<std::string> &checkNames);

private:
    CheckManager();
    static std::mutex m_lock;

    void registerChecks();
    void registerFixIt(int id, const std::string &fititName, const std::string &checkName);
    void registerCheck(const RegisteredCheck &check);
    bool checkExists(const std::string &name) const;
    RegisteredCheck::List checksForLevel(int level) const;
    CheckBase *createCheck(const std::string &name, ClazyContext *context);
    std::string checkNameForFixIt(const std::string &) const;
    RegisteredCheck::List m_registeredChecks;
    std::unordered_map<std::string, std::vector<RegisteredFixIt>> m_fixitsByCheckName;
    std::unordered_map<std::string, RegisteredFixIt> m_fixitByName;
};

#endif
