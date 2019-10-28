#!/usr/bin/env python

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

import sys, os, json, argparse, datetime, io
from shutil import copyfile

CHECKS_FILENAME = 'checks.json'
_checks = []
_specified_check_names = []
_available_categories = []

def checkSortKey(check):
    return str(check.level) + check.name

def level_num_to_enum(n):
    if n == -1:
        return 'ManualCheckLevel'
    if n >= 0 and n <= 3:
        return 'CheckLevel' + str(n)

    return 'CheckLevelUndefined'

def level_num_to_name(n):
    if n == -1:
        return 'Manual Level'
    if n >= 0 and n <= 3:
        return 'Level ' + str(n)

    return 'undefined'

def level_num_to_cmake_readme_variable(n):
    if n == -1:
        return 'README_manuallevel_FILES'
    if n >= 0 and n <= 3:
        return 'README_LEVEL%s_FILES' % str(n)

    return 'undefined'

def clazy_source_path():
    return os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/..") + "/"

def templates_path():
    return clazy_source_path() + "dev-scripts/templates/"

def docs_relative_path():
    return "docs/checks/"

def docs_path():
    return clazy_source_path() + docs_relative_path()

def read_file(filename):
    f = io.open(filename, 'r', newline='\n', encoding='utf8')
    contents = f.read()
    f.close()
    return contents

def write_file(filename, contents):
    f = io.open(filename, 'w', newline='\n', encoding='utf8')
    f.write(contents)
    f.close()

def get_copyright():
    year = datetime.datetime.now().year
    author = os.getenv('GIT_AUTHOR_NAME', 'Author')
    email = os.getenv('GIT_AUTHOR_EMAIL', 'your@email')
    return "Copyright (C) %s %s <%s>" % (year, author, email)

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

    def include(self): # Returns for example: "returning-void-expression.h"
        oldstyle_headername = (self.name + ".h").replace('-', '')
        if os.path.exists(self.path() + oldstyle_headername):
            return oldstyle_headername

        return self.name + '.h'

    def qualified_include(self): # Returns for example: "checks/level2/returning-void-expression.h"
        return self.basedir() + self.include()

    def qualified_cpp_filename(self): # Returns for example: "checks/level2/returning-void-expression.cpp"
        return self.basedir() + self.cpp_filename()

    def cpp_filename(self): # Returns for example: "returning-void-expression.cpp"
        filename = self.include()
        filename = filename.replace(".h", ".cpp")
        return filename

    def path(self):
        return clazy_source_path() + self.basedir(True) + "/"

    def basedir(self, with_src=False):
        level = 'level' + str(self.level)
        if self.level == -1:
            level = 'manuallevel'

        if with_src:
            return "src/checks/" + level + '/'
        return "checks/" + level + '/'

    def readme_name(self):
        return "README-" + self.name + ".md"

    def readme_path(self):
        return docs_path() + self.readme_name()


    def supportsQt4(self):
        return self.minimum_qt_version < 50000

    def get_class_name(self):
        if self.class_name:
            return self.class_name

        # Deduce the class name
        splitted = self.name.split('-')
        classname = ""
        for word in splitted:
            if word == 'qt':
                word = 'Qt'
            else:
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

    def include_guard(self):
        guard = self.name.replace('-', '_')
        return guard.upper()


def load_json(filename):
    jsonContents = read_file(filename)
    decodedJson = json.loads(jsonContents)

    if 'checks' not in decodedJson:
        print("No checks found in " + filename)
        return False

    checks = decodedJson['checks']

    global _available_categories, _checks, _specified_check_names

    if 'available_categories' in decodedJson:
        _available_categories = decodedJson['available_categories']

    for check in checks:
        c = Check()
        try:
            c.name = check['name']
            c.level = check['level']
            if 'categories' in check:
                c.categories = check['categories']
            for cat in c.categories:
                if cat not in _available_categories:
                    print('Unknown category ' + cat)
                    return False
        except KeyError:
            print("Missing mandatory field while processing " + str(check))
            return False

        if _specified_check_names and c.name not in _specified_check_names:
            continue

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

    _checks = sorted(_checks, key=checkSortKey)
    return True

def print_checks(checks):
    for c in checks:
        print(c.name + " " + str(c.level) + " " + str(c.categories))

#-------------------------------------------------------------------------------
def generate_register_checks(checks):
    text = '#include "checkmanager.h"\n'
    for c in checks:
        text += '#include "' + c.qualified_include() + '"\n'
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
    filename = clazy_source_path() + "src/Checks.h"

    old_text = read_file(filename)
    if old_text != text:
        write_file(filename, text)
        print("Generated " + filename)
        return True
    return False
#-------------------------------------------------------------------------------
def generate_cmake_file(checks):
    text = "set(CLAZY_CHECKS_SRCS ${CLAZY_CHECKS_SRCS}\n"
    for level in [-1, 0, 1, 2, 3]:
        for check in checks:
            if check.level == level:
                text += "  ${CMAKE_CURRENT_LIST_DIR}/src/" + check.qualified_cpp_filename() + "\n"
    text += ")\n"

    filename = clazy_source_path() + "CheckSources.cmake"
    old_text = read_file(filename)
    if old_text != text:
        write_file(filename, text)
        print("Generated " + filename)
        return True
    return False
#-------------------------------------------------------------------------------
def create_readmes(checks):
    generated = False
    for check in checks:
        if not os.path.exists(check.readme_path()):
            existing_readme = search_in_all_levels(check.readme_name())
            if existing_readme:
                contents = read_file(existing_readme)
                write_file(check.readme_path(), contents)
                os.remove(existing_readme)
                print("Moved " + check.readme_name())
            else:
                contents = read_file(templates_path() + "check-readme.md")
                contents = contents.replace('[check-name]', check.name)
                write_file(check.readme_path(), contents)
                print("Created " + check.readme_path())
            generated = True
    return generated
#-------------------------------------------------------------------------------
def create_unittests(checks):
    generated = False
    for check in checks:
        unittest_folder = clazy_source_path() + "tests/" + check.name
        if not os.path.exists(unittest_folder):
            os.mkdir(unittest_folder)
            print("Created " + unittest_folder)
            generated = True

        configjson_file = unittest_folder + "/config.json"
        if not os.path.exists(configjson_file):
            copyfile(templates_path() + "test-config.json", configjson_file)
            print("Created " + configjson_file)
            generated = True

        testmain_file = unittest_folder + "/main.cpp"
        if not os.path.exists(testmain_file) and check.name != 'non-pod-global-static':
            copyfile(templates_path() + "test-main.cpp", testmain_file)
            print("Created " + testmain_file)
            generated = True
    return generated

#-------------------------------------------------------------------------------
def search_in_all_levels(filename):
    for level in ['manuallevel', 'level0', 'level1', 'level2']:
        complete_filename = clazy_source_path() + 'src/checks/' + level + '/' + filename
        if os.path.exists(complete_filename):
            return complete_filename
    return ""

#-------------------------------------------------------------------------------
def create_checks(checks):
    generated = False

    for check in checks:
        edit_changelog = False
        include_file = check.path() + check.include()
        cpp_file = check.path() + check.cpp_filename()
        copyright = get_copyright()
        include_missing = not os.path.exists(include_file)
        cpp_missing = not os.path.exists(cpp_file)
        if include_missing:

            existing_include_path = search_in_all_levels(check.include())
            if existing_include_path:
                # File already exists, but is in another level. Just move it:
                contents = read_file(existing_include_path)
                write_file(include_file, contents)
                os.remove(existing_include_path)
                print("Moved " + check.include())
            else:
                contents = read_file(templates_path() + 'check.h')
                contents = contents.replace('%1', check.include_guard())
                contents = contents.replace('%2', check.get_class_name())
                contents = contents.replace('%3', check.name)
                contents = contents.replace('%4', copyright)
                write_file(include_file, contents)
                print("Created " + include_file)
                edit_changelog = True
            generated = True
        if cpp_missing:
            existing_cpp_path = search_in_all_levels(check.cpp_filename())
            if existing_cpp_path:
                # File already exists, but is in another level. Just move it:
                contents = read_file(existing_cpp_path)
                write_file(cpp_file, contents)
                os.remove(existing_cpp_path)
                print("Moved " + check.cpp_filename())
            else:
                contents = read_file(templates_path() + 'check.cpp')
                contents = contents.replace('%1', check.include())
                contents = contents.replace('%2', check.get_class_name())
                contents = contents.replace('%3', copyright)
                write_file(cpp_file, contents)
                print("Created " + cpp_file)
            generated = True

        if edit_changelog:
            # We created a new check, let's also edit the ChangeLog
            changelog_file = clazy_source_path() + 'Changelog'
            contents = read_file(changelog_file)
            contents += '\n  - <dont forget changelog entry for ' + check.name + '>\n'
            write_file(changelog_file, contents)
            print('Edited Changelog')

    return generated
#-------------------------------------------------------------------------------
def generate_readme(checks):
    filename = clazy_source_path() + "README.md"
    f = io.open(filename, 'r', newline='\n', encoding='utf8')
    old_contents = f.readlines();
    f.close();

    new_text_to_insert = ""
    for level in ['-1', '0', '1', '2']:
        new_text_to_insert += "- Checks from %s:" % level_num_to_name(int(level)) + "\n"
        for c in checks:
            if str(c.level) == level:
                fixits_text = c.fixits_text()
                if fixits_text:
                    fixits_text = "    " + fixits_text
                new_text_to_insert += "    - [%s](%sREADME-%s.md)%s" % (c.name, docs_relative_path(), c.name, fixits_text) + "\n"
        new_text_to_insert += "\n"


    f = io.open(filename, 'w', newline='\n', encoding='utf8')

    skip = False
    for line in old_contents:
        if skip and line.startswith("#"):
            skip = False

        if skip:
            continue

        if line.startswith("- Checks from Manual Level:"):
            skip = True
            f.write(new_text_to_insert)
            continue

        f.write(line)
    f.close()

    f = io.open(filename, 'r', newline='\n', encoding='utf8')
    new_contents = f.readlines();
    f.close();

    if old_contents != new_contents:
        print("Generated " + filename)
        return True
    return False
#-------------------------------------------------------------------------------
def generate_readmes_cmake_install(checks):
    old_contents = ""
    filename = clazy_source_path() + 'readmes.cmake'
    if os.path.exists(filename):
        f = io.open(filename, 'r', newline='\n', encoding='utf8')
        old_contents = f.readlines();
        f.close();

    new_text_to_insert = ""
    for level in ['-1', '0', '1', '2']:
        new_text_to_insert += 'SET(' + level_num_to_cmake_readme_variable(int(level)) + "\n"
        for c in checks:
            if str(c.level) == level:
                new_text_to_insert += '    ${CMAKE_CURRENT_LIST_DIR}/docs/checks/' + c.readme_name() + '\n'
        new_text_to_insert += ')\n\n'

        if old_contents == new_text_to_insert:
            return False

    f = io.open(filename, 'w', newline='\n', encoding='utf8')
    f.write(new_text_to_insert)
    f.close()
    return True

#-------------------------------------------------------------------------------

complete_json_filename = clazy_source_path() + CHECKS_FILENAME

if not os.path.exists(complete_json_filename):
    print("File doesn't exist: " + complete_json_filename)
    exit(1)



parser = argparse.ArgumentParser()
parser.add_argument("--generate", action='store_true', help="Generate src/Checks.h, CheckSources.cmake and README.md")
parser.add_argument("checks", nargs='*', help="Optional check names to build. Useful to speedup builds during development, by building only the specified checks. Default is to build all checks.")
args = parser.parse_args()

_specified_check_names = args.checks

if not load_json(complete_json_filename):
    exit(1)

if args.generate:
    generated = False
    generated = generate_register_checks(_checks) or generated
    generated = generate_cmake_file(_checks) or generated
    generated = generate_readme(_checks) or generated
    generated = create_readmes(_checks) or generated
    generated = create_unittests(_checks) or generated
    generated = create_checks(_checks) or generated
    generated = generate_readmes_cmake_install(_checks) or generated
    if not generated:
        print("Nothing to do, everything is OK")
else:
    parser.print_help(sys.stderr)
