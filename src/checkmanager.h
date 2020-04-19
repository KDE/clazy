/*
    This file is part of the clazy static checker.

    Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Sérgio Martins <sergio.martins@kdab.com>

    Copyright (C) 2015-2016 Sergio Martins <smartins@kde.org>

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

#ifndef CLANG_LAZY_CHECK_MANAGER_H
#define CLANG_LAZY_CHECK_MANAGER_H

#include "checkbase.h"

#include <clang/Lex/PreprocessorOptions.h>

#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <utility>
#include <string>

class ClazyContext;

struct RegisteredFixIt {
    typedef std::vector<RegisteredFixIt> List;
    RegisteredFixIt() : id(-1) {}
    RegisteredFixIt(int id, const std::string &name) : id(id), name(name) {}
    int id = -1;
    std::string name;
    bool operator==(const RegisteredFixIt &other) const { return id == other.id; }
};

using FactoryFunction = std::function<CheckBase*(ClazyContext *context)>;

struct RegisteredCheck {
    enum Option {
        Option_None = 0,
        Option_Qt4Incompatible = 1,
        Option_VisitsStmts = 2,
        Option_VisitsDecls = 4
    };

    typedef std::vector<RegisteredCheck> List;
    typedef int Options;

    std::string name;
    CheckLevel level;
    FactoryFunction factory;
    Options options;
    bool operator==(const RegisteredCheck &other) const { return name == other.name; }
};

inline bool checkLessThan(const RegisteredCheck &c1, const RegisteredCheck &c2)
{
    return c1.name < c2.name;
}

inline bool checkLessThanByLevel(const RegisteredCheck &c1, const RegisteredCheck &c2)
{
    if (c1.level == c2.level)
        return checkLessThan(c1, c2);

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

    static std::mutex &lock() { return m_lock; }
    RegisteredCheck::List availableChecks(CheckLevel maxLevel) const;
    RegisteredCheck::List requestedChecksThroughEnv(std::vector<std::string> &userDisabledChecks) const;
    std::vector<std::string> checksAsErrors() const;

    RegisteredCheck::List::const_iterator checkForName(const RegisteredCheck::List &checks, const std::string &name) const;
    RegisteredCheck::List checksForCommaSeparatedString(const std::string &str) const;
    RegisteredCheck::List checksForCommaSeparatedString(const std::string &str,
                                                        std::vector<std::string> &userDisabledChecks) const;
    RegisteredFixIt::List availableFixIts(const std::string &checkName) const;


    /**
     * Returns all the requested checks.
     * This is a union of the requested checks via env variable and via arguments passed to compiler
     */
    RegisteredCheck::List requestedChecks(std::vector<std::string> &args, bool qt4Compat);
    std::vector<std::pair<CheckBase*, RegisteredCheck>> createChecks(const RegisteredCheck::List &requestedChecks, ClazyContext *context);

    static void removeChecksFromList(RegisteredCheck::List &list, std::vector<std::string> &checkNames);

private:
    CheckManager();
    static std::mutex m_lock;

    void registerChecks();
    void registerFixIt(int id, const std::string &fititName, const std::string &checkName);
    void registerCheck(const RegisteredCheck &check);
    bool checkExists(const std::string &name) const;
    RegisteredCheck::List checksForLevel(int level) const;
    CheckBase* createCheck(const std::string &name, ClazyContext *context);
    std::string checkNameForFixIt(const std::string &) const;
    RegisteredCheck::List m_registeredChecks;
    std::unordered_map<std::string, std::vector<RegisteredFixIt>> m_fixitsByCheckName;
    std::unordered_map<std::string, RegisteredFixIt > m_fixitByName;
};

#endif
