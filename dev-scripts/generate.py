#!/usr/bin/env python3

_license_text = \
"""/*
  This file is part of the clazy static checker.

  Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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
"""

import sys, os, json, argparse

CHECKS_FILENAME = 'checks.json'
_checks = []
_available_categories = []

def level_num_to_enum(n):
    if n == -1:
        return 'ManualCheckLevel'
    if n >= 0 and n <= 3:
        return 'CheckLevel' + str(n)

    return 'CheckLevelUndefined'

class Check:
    def __init__(self):
        self.name = ""
        self.class_name = ""
        self.level = 0
        self.categories = []
        self.minimum_qt_version = 40000 # Qt 4.0.0
        self.fixits = []
        self.visits_stmts = False
        self.visits_decls = False
        self.ifndef = ""

    def include(self):
        level = 'level' + str(self.level)
        if self.level == -1:
            level = 'manuallevel'
        headername = self.name + ".h"
        filename = 'checks/' + level + '/' + headername
        if not os.path.exists('src/' + filename):
            filename = filename.replace('-', '')

        return filename

    def supportsQt4(self):
        return self.minimum_qt_version < 50000

    def get_class_name(self):
        if self.class_name:
            return self.class_name

        # Deduce the class name
        splitted = self.name.split('-')
        classname = ""
        for word in splitted:
            word = word.title()
            if word.startswith('Q'):
                word = 'Q' + word[1:].title()

            classname += word

        return classname

    def valid_name(self):
        if self.name in ['clazy']:
            return False
        if self.name.startswith('level'):
            return False
        if self.name.startswith('fix'):
            return False
        return True

    def fixits_text(self):
        if not self.fixits:
            return ""

        text = ""
        fixitnames = []
        for f in self.fixits:
            fixitnames.append("fix-" + f)

        text = ','.join(fixitnames)

        return "(" + text + ")"

if not os.path.exists(CHECKS_FILENAME):
    print("File doesn't exist: " + CHECKS_FILENAME)
    exit(1)

def load_json(filename):
    f = open(filename, 'r')
    jsonContents = f.read()
    f.close()
    decodedJson = json.loads(jsonContents)

    if 'checks' not in decodedJson:
        print("No checks found in " + filename)
        return False

    checks = decodedJson['checks']

    global _available_categories
    if 'available_categories' in decodedJson:
        _available_categories = decodedJson['available_categories']

    for check in checks:
        c = Check()
        try:
            c.name = check['name']
            c.level = check['level']
            c.categories = check['categories']
            for cat in c.categories:
                if cat not in _available_categories:
                    print('Unknown category ' + cat)
                    return False
        except KeyError:
            print("Missing mandatory field while processing " + str(check))
            return False

        if 'class_name' in check:
            c.class_name = check['class_name']

        if 'ifndef' in check:
            c.ifndef = check['ifndef']

        if 'minimum_qt_version' in check:
            c.minimum_qt_version = check['minimum_qt_version']

        if 'visits_stmts' in check:
            c.visits_stmts = check['visits_stmts']

        if 'visits_decls' in check:
            c.visits_decls = check['visits_decls']

        if 'fixits' in check:
            for fixit in check['fixits']:
                if 'name' not in fixit:
                    print('fixit doesnt have a name. check=' + str(check))
                    return False
                c.fixits.append(fixit['name'])

        if not c.valid_name():
            print("Invalid check name: %s" % (c.name()))
            return False
        _checks.append(c)

    return True

def print_checks(checks):
    for c in checks:
        print(c.name + " " + str(c.level) + " " + str(c.categories))

def generate_register_checks(checks):
    text = '#include "checkmanager.h"\n'
    for c in checks:
        text += '#include "' + c.include() + '"\n'
    text += \
"""
template <typename T>
RegisteredCheck check(const char *name, CheckLevel level, RegisteredCheck::Options options = RegisteredCheck::Option_None)
{
    auto factoryFuntion = [name](ClazyContext *context){ return new T(name, context); };
    return RegisteredCheck{name, level, factoryFuntion, options};
}

void CheckManager::registerChecks()
{
"""

    for c in checks:
        qt4flag = "RegisteredCheck::Option_None"
        if not c.supportsQt4():
            qt4flag = "RegisteredCheck::Option_Qt4Incompatible"

        if c.visits_stmts:
            qt4flag += " | RegisteredCheck::Option_VisitsStmts"
        if c.visits_decls:
            qt4flag += " | RegisteredCheck::Option_VisitsDecls"

        qt4flag = qt4flag.replace("RegisteredCheck::Option_None |", "")

        if c.ifndef:
            text += "#ifndef " + c.ifndef + "\n"

        text += '    registerCheck(check<%s>("%s", %s, %s));\n' % (c.get_class_name(), c.name, level_num_to_enum(c.level), qt4flag)

        fixitID = 1
        for fixit in c.fixits:
            text += '    registerFixIt(%d, "%s", "%s");\n' % (fixitID, "fix-" + fixit, c.name)
            fixitID = fixitID * 2

        if c.ifndef:
            text += "#endif" + "\n"

    text += "}\n"

    comment_text = \
"""
/**
 * To add a new check you can either edit this file, or use the python script:
 * dev-scripts/generate.py > src/Checks.h
 */
"""
    text = _license_text + '\n' + comment_text + '\n' + text
    print(text)

def print_markdown_help():
    for level in ['0', '1', '2', '3']:
        print("\n- Checks from level%s:" % level)
        for c in _checks:
            if str(c.level) == level:
                fixits_text = c.fixits_text()
                if fixits_text:
                    fixits_text = "    " + fixits_text
                print("    - [%s](src/checks/level%s/README-%s.md)%s" % (c.name, level, c.name, fixits_text))


if not load_json(CHECKS_FILENAME):
    exit(1)

parser = argparse.ArgumentParser()
parser.add_argument("--generate-readme", action='store_true', help="Generate list of checks to copy-paste to README.md")
parser.add_argument("--generate-checks-header", action='store_true', help="Generate src/Checks.h")
args = parser.parse_args()

if args.generate_readme:
    print_markdown_help()
elif args.generate_checks_header:
    generate_register_checks(_checks)

#print_checks(_checks)
