#!/usr/bin/env python3

import sys
import os
import subprocess
import re
import json
import threading
import multiprocessing
import argparse
import io
import shutil
from threading import Thread
from sys import platform as _platform
from pathlib import Path
import platform

# cd into the folder containing this script
os.chdir(os.path.realpath(os.path.dirname(sys.argv[0])))

_verbose = False

c_headerpath = False
try:
    result = subprocess.run(['clang', '-v'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, text=True)
    match = re.search(r'Selected .* installation: (.*)', result.stderr)
    if match:
        c_headerpath = match.group(1).strip()
except:
    pass


def isWindows():
    return _platform == 'win32'

def isMacOS():
    return _platform == 'darwin'

def isLinux():
    return _platform.startswith('linux')

class QtInstallation:
    def __init__(self):
        self.int_version = 000
        self.qmake_header_path = "/usr/include/qt/"
        self.qmake_lib_path = "/usr/lib"

    def compiler_flags(self, module_includes = False):
        extra_includes = ''
        if isMacOS():
            extra_includes = " -I%s/QtCore.framework/Headers" % self.qmake_lib_path
            extra_includes += " -iframework %s" % self.qmake_lib_path

        # Also include the modules folders
        qt_modules_includes = []
        if module_includes:
            qt_modules_includes = ["-isystem " + self.qmake_header_path + "/" + f for f in next(os.walk(self.qmake_header_path))[1]]
        additonal_args = ""
        if c_headerpath:
            additonal_args = "-isystem " + c_headerpath + "/include "
        if _cxx_args:
            additonal_args += _cxx_args + " "
        if _qt_namespaced:
            additonal_args += " -DQT_NAMESPACE=MyQt "

        return additonal_args + "-isystem " + self.qmake_header_path + ("" if isWindows() else " -fPIC") + " -L " + self.qmake_lib_path + ' ' + extra_includes + ' '.join(qt_modules_includes)


class Test:
    def __init__(self, check):
        self.filenames = []
        self.minimum_qt_version = 500
        self.maximum_qt_version = 69999
        self.minimum_clang_version = 380
        self.minimum_clang_version_for_fixits = 380
        self.compare_everything = False
        self.check = check
        self.expects_failure = False
        self.qt_major_versions = [5, 6]
        self.env = os.environ
        self.checks = []
        self.flags = ""
        self.must_fail = False
        self.blacklist_platforms = []
        self.only_qt = False
        self.qt_developer = False
        self.header_filter = ""
        self.ignore_dirs = ""
        self.has_fixits = False
        self.should_run_fixits_test = False
        self.should_run_on_32bit = True
        self.cppStandards = ["c++14", "c++17"]
        self.extra_definitions = False
        self.qt_modules_includes = False
        self.fixed_file_base = None

    def filename(self):
        if len(self.filenames) == 1:
            return self.filenames[0]
        return ""

    def relativeFilename(self):
        # example: "auto-unexpected-qstringbuilder/main.cpp"
        return self.check.name + "/" + self.filename()

    def yamlFilename(self, is_standalone):
        # The name of the yaml file with fixits
        # example: "auto-unexpected-qstringbuilder/main.cpp.clazy.yaml"
        if is_standalone:
            return self.relativeFilename() + ".clazy-standalone.yaml"
        else:
            return self.relativeFilename() + ".clazy.yaml"

    def fixedFilename(self, is_standalone):
        if is_standalone:
            return self.relativeFilename() + ".clazy-standalone.fixed"
        else:
            return self.relativeFilename() + ".clazy.fixed"

    def expectedFixedFilename(self):
        return self.relativeFilename() + ".fixed.expected"

    def isScript(self):
        return self.filename().endswith(".sh")

    def dir(self):
        return self.check.name

    def setQtMajorVersions(self, major_versions):
        self.qt_major_versions = major_versions
        if 4 in major_versions:
            if self.minimum_qt_version >= 500:
                self.minimum_qt_version = 400

    def envString(self):
        result = ""
        for key in self.env:
            result += key + '="' + self.env[key] + '" '
        return result

    def setEnv(self, e):
        self.env = os.environ.copy()
        for key in e:
            if type(key) is bytes:
                key = key.decode('utf-8')

            self.env[key] = e[key]

    def printableName(self, cppStandard, qt_major_version, is_standalone, is_tidy, is_fixits):
        name = self.check.name
        if len(self.check.tests) > 1:
            name += "/" + self.filename()
        if len(cppStandard) > 0:
            name += " (" + cppStandard + ")"
        if qt_major_version > 0:
            name += " (Qt " + str(qt_major_version) + ")"
        if is_fixits and is_standalone:
            name += " (standalone, fixits)"
        elif is_standalone:
            name += " (standalone)"
        elif is_tidy:
            name += " (clang-tidy)"
        elif is_fixits:
            name += " (plugin, fixits)"
        else:
            name += " (plugin)"
        return name

    def removeYamlFiles(self):
        for f in [self.yamlFilename(False), self.yamlFilename(True)]:
            if os.path.exists(f):
                os.remove(f)


class Check:
    def __init__(self, name):
        self.name = name
        self.minimum_clang_version = 380  # clang 3.8.0
        self.minimum_qt_version = 500
        self.maximum_qt_version = 69999
        self.enabled = True
        self.clazy_standalone_only = False
        self.tests = []
# -------------------------------------------------------------------------------
# utility functions #1


def get_command_output(cmd: str, test_env=os.environ, cwd=None, ignore_verbose=False):
    success = True

    try:
        if _verbose and not ignore_verbose:
            print(cmd)

        # Polish up the env to fix "TypeError: environment can only contain strings" exception
        str_env = {}
        for key in test_env.keys():
            str_env[str(key)] = str(test_env[key])

        output = subprocess.check_output(
            cmd, stderr=subprocess.STDOUT, shell=True, env=str_env, cwd=cwd)
    except subprocess.CalledProcessError as e:
        output = e.output
        success = False

    if type(output) is bytes:
        output = output.decode('utf-8')

    return output, success


def load_json(check_name: str):
    check = Check(check_name)
    filename = check_name + "/config.json"
    if not os.path.exists(filename):
        # Ignore this directory
        return check

    f = open(filename, 'r')
    contents = f.read()
    f.close()
    decoded = json.loads(contents)
    check_blacklist_platforms = []

    if 'minimum_clang_version' in decoded:
        check.minimum_clang_version = decoded['minimum_clang_version']

    if 'minimum_qt_version' in decoded:
        check.minimum_qt_version = decoded['minimum_qt_version']

    if 'maximum_qt_version' in decoded:
        check.maximum_qt_version = decoded['maximum_qt_version']

    if 'enabled' in decoded:
        check.enabled = decoded['enabled']

    if 'clazy_standalone_only' in decoded:
        check.clazy_standalone_only = decoded['clazy_standalone_only']

    if 'blacklist_platforms' in decoded:
        check_blacklist_platforms = decoded['blacklist_platforms']

    if 'tests' in decoded:
        for t in decoded['tests']:
            test = Test(check)
            test.blacklist_platforms = check_blacklist_platforms

            if 'filename' in t:
                test.filenames.append(t['filename'])

            if 'filenames' in t:
                test.filenames += t['filenames']

            if 'minimum_qt_version' in t:
                test.minimum_qt_version = t['minimum_qt_version']
            else:
                test.minimum_qt_version = check.minimum_qt_version

            if 'maximum_qt_version' in t:
                test.maximum_qt_version = t['maximum_qt_version']
            else:
                test.maximum_qt_version = check.maximum_qt_version

            if 'minimum_clang_version' in t:
                test.minimum_clang_version = t['minimum_clang_version']
            else:
                test.minimum_clang_version = check.minimum_clang_version

            if 'minimum_clang_version_for_fixits' in t:
                test.minimum_clang_version_for_fixits = t['minimum_clang_version_for_fixits']

            if 'blacklist_platforms' in t:
                test.blacklist_platforms = t['blacklist_platforms']
            if 'compare_everything' in t:
                test.compare_everything = t['compare_everything']
            if 'qt_major_versions' in t:
                test.setQtMajorVersions(t['qt_major_versions'])
            if 'env' in t:
                test.setEnv(t['env'])
            if 'checks' in t:
                test.checks = t['checks']
            if 'flags' in t:
                test.flags = t['flags']
            if 'must_fail' in t:
                test.must_fail = t['must_fail']
            if 'has_fixits' in t:
                test.has_fixits = t['has_fixits'] and test.minimum_clang_version_for_fixits <= CLANG_VERSION
            if 'expects_failure' in t:
                test.expects_failure = t['expects_failure']
            if 'only_qt' in t:
                test.only_qt = t['only_qt']
            if 'cppStandards' in t:
                test.cppStandards = t['cppStandards']
            if 'qt_developer' in t:
                test.qt_developer = t['qt_developer']
            if 'extra_definitions' in t:
                test.extra_definitions = " " + t['extra_definitions'] + " "
            if 'header_filter' in t:
                test.header_filter = t['header_filter']
            if 'ignore_dirs' in t:
                test.ignore_dirs = t['ignore_dirs']
            if 'should_run_on_32bit' in t:
                test.should_run_on_32bit = t['should_run_on_32bit']
            if 'qt_modules_includes' in t:
                test.qt_modules_includes = t['qt_modules_includes']

            if not test.checks:
                test.checks.append(test.check.name)

            check.tests.append(test)

    return check


def find_qt_installation(major_version, qmakes):
    installation = QtInstallation()

    for qmake in qmakes:
        qmake_version_str, success = get_command_output(qmake + " -query QT_VERSION")
        if success and qmake_version_str.startswith(str(major_version) + "."):
            qmake_header_path = get_command_output(
                qmake + " -query QT_INSTALL_HEADERS")[0].strip()
            qmake_lib_path = get_command_output(
                qmake + " -query QT_INSTALL_LIBS")[0].strip()
            if qmake_header_path:
                installation.qmake_header_path = qmake_header_path
                if qmake_lib_path:
                    installation.qmake_lib_path = qmake_lib_path
                ver = qmake_version_str.split('.')
                installation.int_version = int(
                    ver[0]) * 10000 + int(ver[1]) * 100 + int(ver[2])
                if _verbose:
                    print("Found Qt " + str(installation.int_version) +
                          " using qmake " + qmake)
            break

    if installation.int_version == 0:
        print("Error: Couldn't find a Qt" + str(major_version) + " installation")
    return installation


def libraryName():
    if 'CLAZYPLUGIN_CXX' in os.environ: # Running tests uninstalled
        return os.environ['CLAZYPLUGIN_CXX']
    elif _platform == 'win32':
        return 'ClazyPlugin.dll'
    elif _platform == 'darwin':
        return 'ClazyPlugin.dylib'
    else:
        return 'ClazyPlugin.so'


def clangTidyPluginName():
    if 'CLANGTIDYPLUGIN_CXX' in os.environ: # Running tests uninstalled
        return os.environ['CLANGTIDYPLUGIN_CXX']
    return "ClazyClangTidy.so"


def clazy_cpp_args(cppStandard):
    return ' -Wno-unused-value -Qunused-arguments -std=' + cppStandard + ' '


def clazy_standalone_binary():
    if 'CLAZYSTANDALONE_CXX' in os.environ:  # in case we want to use "clazy.AppImage --standalone" instead
        return os.environ['CLAZYSTANDALONE_CXX']
    return 'clazy-standalone'


def more_clazy_standalone_args():
    if 'CLANG_BUILTIN_INCLUDE_DIR' in os.environ:
        return ' -I ' + os.environ['CLANG_BUILTIN_INCLUDE_DIR']
    return ''


def clazy_standalone_command(test: Test, cppStandard, qt):
    result = " -- " + clazy_cpp_args(cppStandard) + \
        qt.compiler_flags(test.qt_modules_includes) + " " + test.flags + more_clazy_standalone_args()
    result = " -checks=" + ','.join(test.checks) + " " + result + suppress_line_numbers_opt

    if test.has_fixits:
        result = " -export-fixes=" + \
            test.yamlFilename(is_standalone=True) + result

    if test.only_qt:
        result = " -only-qt " + result

    if test.qt_developer:
        result = " -qt-developer " + result

    if test.header_filter:
        result = " -header-filter " + test.header_filter + " " + result

    if test.ignore_dirs:
        result = " -ignore-dirs " + test.ignore_dirs + " " + result

    if test.extra_definitions:
        result += test.extra_definitions

    return result


def clang_name():
    return os.getenv('CLANGXX', 'clang')


def clazy_command(test: Test, cppStandard, qt, filename):
    if test.isScript():
        return filename # is absolte path

    if 'CLAZY_CXX' in os.environ:  # In case we want to use clazy.bat
        result = os.environ['CLAZY_CXX']
    else:
        result = clang_name() + " -Xclang -load -Xclang " + libraryName() + " -Xclang -add-plugin -Xclang clazy " 
    result += clazy_cpp_args(cppStandard) + qt.compiler_flags(test.qt_modules_includes) + suppress_line_numbers_opt 

    if test.only_qt:
        result = result + " -Xclang -plugin-arg-clazy -Xclang only-qt "
    if test.qt_developer:
        result = result + " -Xclang -plugin-arg-clazy -Xclang qt-developer "
    if test.extra_definitions:
        result += test.extra_definitions

    result = result + " -c "

    result = result + test.flags + \
        " -Xclang -plugin-arg-clazy -Xclang " + ','.join(test.checks) + " "
    if test.has_fixits:
        result += _export_fixes_argument + " "
    result += filename

    return result


def clang_tidy_command(test: Test, cppStandard, qt, filename):
    command = f"clang-tidy {filename}"
    # disable all checks, re-enable clazy ones
    checks = ','.join("clazy-" + check for check in test.checks)
    command += f" -checks='-*,{checks}' -header-filter='.*' -system-headers -load='{clangTidyPluginName()}'"

    # Add extra compiler flags
    command += " -- "
    command += f" {test.flags} "
    command += clazy_cpp_args(cppStandard)
    command += qt.compiler_flags(test.qt_modules_includes)
    command += suppress_line_numbers_opt
    if test.extra_definitions:
        command += test.extra_definitions
    return command


def dump_ast_command(test: Test, cppStandard, qt_major_version):
    return "clang -std=" + cppStandard + " -fsyntax-only -Xclang -ast-dump -fno-color-diagnostics -c " + qt_installation(qt_major_version).compiler_flags(test.qt_modules_includes) + " " + test.flags + " " + test.filename()


def compiler_name():
    if 'CLAZY_CXX' in os.environ:
        return os.environ['CLAZY_CXX']  # so we can set clazy.bat instead
    return os.getenv('CLANGXX', 'clang')

# -------------------------------------------------------------------------------
# Setup argparse


parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action='store_true')
parser.add_argument("--no-standalone", action='store_true',
                    help="Don\'t run clazy-standalone")
parser.add_argument("--no-clang-tidy", action='store_true',
                    help="Don\'t run clang-tidy")
parser.add_argument("--no-fixits", action='store_true',
                    help='Don\'t run fixits')
parser.add_argument("--only-standalone", action='store_true',
                    help='Only run clazy-standalone')
parser.add_argument("--dump-ast", action='store_true',
                    help='Dump a unit-test AST to file')
parser.add_argument("--qt-versions", type=int, choices=[5, 6], nargs='+', default=[5, 6],
                    help='Specify one or more Qt versions to use (default: 5 and 6)')
parser.add_argument(
    "--exclude", help='Comma separated list of checks to ignore')
parser.add_argument("-j", "--jobs", type=int, default=multiprocessing.cpu_count(),
                    help='Parallel jobs to run (defaults to %(default)s)')
parser.add_argument("check_names", nargs='*',
                    help="The name of the check whose unit-tests will be run. Defaults to running all checks.")
parser.add_argument("--cxx-args", type=str, default="",
                    help="Compiler arguments as a single string")
parser.add_argument("--qt-namespaced", action="store_true",
                    help="Compile tests pretending Qt is namespaced using QT_NAMESPACE")
args = parser.parse_args()

if args.only_standalone and args.no_standalone:
    print("Error: --only-standalone is incompatible with --no-standalone")
    sys.exit(1)

# -------------------------------------------------------------------------------
# Global variables

_export_fixes_argument = "-Xclang -plugin-arg-clazy -Xclang export-fixes"
_dump_ast = args.dump_ast
_verbose = args.verbose
_no_standalone = args.no_standalone
_no_clang_tidy = args.no_clang_tidy
_no_fixits = args.no_fixits
_only_standalone = args.only_standalone
_num_threads = args.jobs
_lock = threading.Lock()
_was_successful = True
_cxx_args = args.cxx_args
_qt_namespaced = args.qt_namespaced
if 6 in args.qt_versions:
    _qt6_installation = find_qt_installation(6, ["QT_SELECT=6 qmake", "qmake-qt6", "qmake", "qmake6"])
else:
    _qt6_installation = None
if 5 in args.qt_versions:
    _qt5_installation = find_qt_installation(5, ["QT_SELECT=5 qmake", "qmake-qt5", "qmake", "qmake5"])
else:
    _qt5_installation = None
if (_qt5_installation is None or _qt5_installation.int_version == 0) \
    and (_qt6_installation is None or _qt6_installation.int_version == 0):
    sys.exit(1)
_excluded_checks = args.exclude.split(',') if args.exclude is not None else []

# -------------------------------------------------------------------------------
# utility functions #2

version, success = get_command_output(compiler_name() + ' --version')
match = re.search('clang version ([^\\s-]+)', version)
try:
    version = match.group(1)
except:
    # Now try the Clazy.AppImage way
    match = re.search('clang version: (.*)', version)
    try:
        version = match.group(1)
    except:
        splitted = version.split()
        if len(splitted) > 2:
            version = splitted[2]
        else:
            print("Could not determine clang version, is it in PATH?")
            sys.exit(-1)

if _verbose:
    print('Found clang version: ' + str(version))

CLANG_VERSION = int(version.replace('git', '').replace('.', ''))

suppress_line_numbers_opt = " -fno-diagnostics-show-line-numbers"


def qt_installation(major_version):
    if major_version == 6:
        return _qt6_installation
    elif major_version == 5:
        return _qt5_installation
    return None


def run_command(cmd, output_file="", test_env=os.environ, cwd=None, ignore_verbose_command=False, qt_namespaced=False, qt_replace_namespace=True):
    lines, success = get_command_output(cmd, test_env, cwd=cwd, ignore_verbose=ignore_verbose_command)
    # Hack for Windows, we have std::_Vector_base in the expected data
    lines = lines.replace("std::_Container_base0", "std::_Vector_base")
    lines = lines.replace("std::__1::__vector_base_common",
                          "std::_Vector_base")  # Hack for macOS
    lines = lines.replace("std::_Vector_alloc", "std::_Vector_base")

    # clang-tidy prints the tags slightly different
    if cmd.startswith("clang-tidy"):
        lines = lines.replace("[clazy", "[-Wclazy")
    if qt_namespaced and qt_replace_namespace:
        lines = lines.replace("MyQt::", "")

    if not success and not output_file:
        print(lines)
        return False

    if _verbose:
        print("Running: " + cmd)
        print("output_file=" + output_file)

    lines = lines.replace('\r\n', '\n')
    if lines.endswith('\n\n'):
        lines = lines[:-1]  # remove *only* one extra newline, all testfiles have a newline based on unix convention
    if output_file:
        f = io.open(output_file, 'w', encoding='utf8')
        f.writelines(lines)
        f.close()
    elif len(lines) > 0:
        print(lines)

    return success


def files_are_equal(file1, file2):
    try:
        f = io.open(file1, 'r', encoding='utf-8')
        lines1 = f.readlines()
        f.close()

        f = io.open(file2, 'r', encoding='utf-8')
        lines2 = f.readlines()
        f.close()

        return lines1 == lines2
    except Exception as ex:
        print("Error comparing files:" + str(ex))
        return False


def compare_files(expects_failure, expected_file, result_file, message):
    success = files_are_equal(expected_file, result_file)

    if expects_failure:
        if success:
            print("[XOK]   " + message)
            return False
        else:
            print("[XFAIL] " + message)
            print_differences(expected_file, result_file)
            return True
    else:
        if success:
            print("[OK]   " + message)
            return True
        else:
            print("[FAIL] " + message)
            print_differences(expected_file, result_file)
            return False


def get_check_names():
    return list(filter(lambda entry: os.path.isdir(entry), os.listdir(".")))

# The yaml file references the test file in our git repo, but we don't want
# to rewrite that one, as we would need to discard git changes afterwards,
# so patch the yaml file and add a ".fixed" suffix to those files


def patch_fixit_yaml_file(test, is_standalone):

    yamlfilename = test.yamlFilename(is_standalone)
    fixedfilename = test.fixedFilename(is_standalone)

    f = open(yamlfilename, 'r')
    lines = f.readlines()
    f.close()
    f = open(yamlfilename, 'w')

    possible_headerfile = test.relativeFilename().replace(".cpp", ".h")

    for line in lines:
        stripped = line.strip()
        if stripped.startswith('MainSourceFile') or stripped.startswith("FilePath") or stripped.startswith("- FilePath"):
            line = line.replace(test.relativeFilename(), fixedfilename)

            # For Windows:
            line = line.replace(test.relativeFilename().replace(
                '/', '\\'), fixedfilename.replace('/', '\\'))

            # Some tests also apply fix their to their headers:
            if not test.relativeFilename().endswith(".hh"):
                line = line.replace(possible_headerfile,
                                    fixedfilename.replace(".cpp", ".h"))
        f.write(line)
    f.close()

    shutil.copyfile(test.relativeFilename(), fixedfilename)

    if os.path.exists(possible_headerfile):
        shutil.copyfile(possible_headerfile,
                        fixedfilename.replace(".cpp", ".h"))

    return True


def run_clang_apply_replacements(check):
    command = os.getenv('CLAZY_CLANG_APPLY_REPLACEMENTS',
                        'clang-apply-replacements')
    return run_command(command + ' ' + check.name)


def cleanup_fixit_files(checks):
    for check in checks:
        filestodelete = list(filter(lambda entry: entry.endswith(
            '.fixed') or entry.endswith('.yaml'), os.listdir(check.name)))
        for f in filestodelete:
            os.remove(check.name + '/' + f)


def print_differences(file1, file2):
    # Returns true if the the files are equal
    return run_command("diff -Naur --strip-trailing-cr {} {}".format(file1, file2))


def normalizedCwd():
    if _platform.startswith('linux'):
        return subprocess.check_output("pwd -L", shell=True, universal_newlines=True).rstrip('\n')
    else:
        return os.getcwd().replace('\\', '/')


def extract_word(word, in_file, out_file):
    in_f = io.open(in_file, 'r', encoding='utf-8')
    out_f = io.open(out_file, 'w', encoding='utf-8')
    for line in in_f:
        if '[-Wdeprecated-declarations]' in line:
            continue

        if word in line:
            line = line.replace('\\', '/')
            # clazy-standalone prints the complete cpp file path for some reason. Normalize it so it compares OK with the expected output.
            line = line.replace(f"{normalizedCwd()}/", "")
            out_f.write(line)
    in_f.close()
    out_f.close()


def print_file(filename):
    f = open(filename, 'r')
    print(f.read())
    f.close()


def file_contains(filename, text):
    f = io.open(filename, 'r', encoding='utf-8')
    contents = f.read()
    f.close()
    return text in contents


def is32Bit():
    return platform.architecture()[0] == '32bit'


def run_unit_test(test, is_standalone, is_tidy, cppStandard, qt_major_version, qt_namespaced):
    if test.check.clazy_standalone_only and not is_standalone:
        return True

    qt = qt_installation(qt_major_version)
    if qt is None:
        return True  # silently skip

    if is_tidy and (test.isScript() or test.only_qt or test.qt_developer):
        print("Options not supported with clang-tidy")
        return True

    if _verbose:
        print("Qt major versions required by the test: " + str(test.qt_major_versions))
        print("Currently considering Qt major version: " + str(qt_major_version))
        print("Qt versions required by the test: min=" + str(test.minimum_qt_version) + " max=" + str(test.maximum_qt_version))
        print("Qt int version: " + str(qt.int_version))
        print("Qt headers: " + qt.qmake_header_path)

    printableName = test.printableName(cppStandard, qt_major_version, is_standalone, is_tidy, False)

    if qt.int_version < test.minimum_qt_version or qt.int_version > test.maximum_qt_version or CLANG_VERSION < test.minimum_clang_version:
        if (_verbose):
            print(f"Skipping {printableName}because required version is not available")
        return True

    if _platform in test.blacklist_platforms:
        if (_verbose):
            print(f"Skipping {printableName} because it is blacklisted for this platform")
        return True

    if not test.should_run_on_32bit and is32Bit():
        if (_verbose):
            print(f"Skipping {printableName} because it is blacklisted on 32bit")
        return True

    checkname = test.check.name
    filename = checkname + "/" + test.filename()
    # Easy copying of command to reproduce manually. Saves me a ton of work - Alex
    abs_filename = str(Path(filename).absolute())

    output_file = filename + ".out"
    result_file = filename + ".result"
    expected_file = filename + ".expected"
    expected_file_tidy = filename + ".expected.tidy"
    expected_file_namespaced = filename + ".expected.qtnamespaced"
    qt_namespaced_separate_file = False
    if qt_namespaced and os.path.exists(expected_file_namespaced):
        expected_file = expected_file_namespaced
        qt_namespaced_separate_file = True
    if is_tidy and os.path.exists(expected_file_tidy):
        expected_file = expected_file_tidy
    elif not os.path.exists(expected_file):
        expected_file = filename + ".qt" + str(qt_major_version) + ".expected"

    # Some tests have different output on 32 bit
    if is32Bit() and os.path.exists(expected_file + '.x86'):
        expected_file = expected_file + '.x86'

    if is_standalone and test.isScript():
        return True

    if is_standalone:
        cmd_to_run = clazy_standalone_binary() + " " + abs_filename + " " + \
            clazy_standalone_command(test, cppStandard, qt)
    elif is_tidy:
        cmd_to_run = clang_tidy_command(test, cppStandard, qt, abs_filename)
    else:
        cmd_to_run = clazy_command(test, cppStandard, qt, abs_filename)

    if test.compare_everything:
        result_file = output_file

    must_fail = test.must_fail

    cmd_success = run_command(cmd_to_run, output_file, test.env, ignore_verbose_command=True, qt_namespaced=qt_namespaced, qt_replace_namespace=not qt_namespaced_separate_file)

    if file_contains(output_file, 'Invalid check: '):
        return True

    if (not cmd_success and not must_fail) or (cmd_success and must_fail):
        print(f"[FAIL] {printableName} (Failed to build test. Check {output_file} for details)")
        print("-------------------")
        print(f"Contents of {output_file}:")
        print_file(output_file)
        print("-------------------")
        return False

    if not test.compare_everything:
        word_to_grep = "warning:" if not must_fail else "error:"
        extract_word(word_to_grep, output_file, result_file)

    # Check that it printed the expected warnings
    if not compare_files(test.expects_failure, expected_file, result_file, printableName):
        return False

    if test.has_fixits and not is_tidy and not _qt_namespaced:
        # The normal tests succeeded, we can run the respective fixits then
        test.should_run_fixits_test = True

    return True


def run_unit_test_for_each_configuration(test, is_standalone, is_tidy, qt_namespaced):
    if test.check.clazy_standalone_only and not is_standalone:
        return True
    result = True
    for qt_major_version in test.qt_major_versions:
        for cppStandard in test.cppStandards:
            if cppStandard == "c++14" and qt_major_version == 6: # Qt6 requires C++17
                continue
            if cppStandard == "c++17" and qt_major_version == 5 and len(test.cppStandards) > 1: # valid combination but let's skip it unless it was the only specified standard
                continue
            result = result and run_unit_test(test, is_standalone, is_tidy, cppStandard, qt_major_version, qt_namespaced)
    return result


def run_unit_tests(tests):
    result = True
    for test in tests:
        test_result = True
        if not _only_standalone:
            test_result = run_unit_test_for_each_configuration(test, False, False, _qt_namespaced)
            result = result and test_result

        if not _no_standalone:
            test_result = test_result and run_unit_test_for_each_configuration(test, True, False, _qt_namespaced)
            result = result and test_result

        if not _no_clang_tidy:
            test_result = test_result and run_unit_test_for_each_configuration(test, False, True, _qt_namespaced)
            result = result and test_result

        if not test_result:
            test.removeYamlFiles()

    global _was_successful, _lock
    with _lock:
        _was_successful = _was_successful and result


def patch_yaml_files(requested_checks, is_standalone):
    if (is_standalone and _no_standalone) or (not is_standalone and _only_standalone):
        # Nothing to do
        return True

    success = True
    for check in requested_checks:
        for test in check.tests:
            if test.should_run_fixits_test:
                yamlfilename = test.yamlFilename(is_standalone)
                if not os.path.exists(yamlfilename):
                    print("[FAIL] " + yamlfilename + " is missing!!")
                    success = False
                    continue
                if not patch_fixit_yaml_file(test, is_standalone):
                    print("[FAIL] Could not patch " + yamlfilename)
                    success = False
                    continue
    return success


def compare_fixit_results(test, is_standalone):

    if (is_standalone and _no_standalone) or (not is_standalone and _only_standalone):
        # Nothing to do
        return True

    # Check that the rewritten file is identical to the expected one
    if not compare_files(False, test.expectedFixedFilename(), test.fixedFilename(is_standalone), test.printableName("", 0, is_standalone, False, True)):
        return False

    # Some fixed cpp files have an header that was also fixed. Compare it here too.
    possible_headerfile_expected = test.expectedFixedFilename().replace('.cpp', '.h')
    if os.path.exists(possible_headerfile_expected):
        possible_headerfile = test.fixedFilename(is_standalone).replace('.cpp', '.h')
        if not compare_files(False, possible_headerfile_expected, possible_headerfile, test.printableName("", 0, is_standalone, False, True).replace('.cpp', '.h')):
            return False

    return True

# This is run sequentially, due to races. As clang-apply-replacements just applies all .yaml files it can find.
# We run a single clang-apply-replacements invocation, which changes all files in the tests/ directory.


def run_fixit_tests(requested_checks):

    success = patch_yaml_files(requested_checks, is_standalone=False)
    success = patch_yaml_files(
        requested_checks, is_standalone=True) and success

    for check in requested_checks:

        if not any(map(lambda test: test.should_run_fixits_test, check.tests)):
            continue

        # Call clazy-apply-replacements[.exe]
        if not run_clang_apply_replacements(check):
            return False

        # Now compare all the *.fixed files with the *.fixed.expected counterparts
        for test in check.tests:
            if test.should_run_fixits_test:
                # Check that the rewritten file is identical to the expected one
                if not compare_fixit_results(test, is_standalone=False):
                    success = False
                    continue

                if not compare_fixit_results(test, is_standalone=True):
                    success = False
                    continue

    return success


def dump_ast(check):
    for test in check.tests:
        for cppStandard in test.cppStandards:
            for version in test.qt_major_versions:
                if version == 6 and cppStandard == "c++14":
                    continue # Qt6 requires C++17
                ast_filename = test.filename() + f"_{cppStandard}_{version}.ast"
                run_command(dump_ast_command(test, cppStandard, version) + " > " + ast_filename)
                print("Dumped AST to " + os.getcwd() + "/" + ast_filename)


def load_checks(all_check_names):
    checks = []
    for name in all_check_names:
        try:
            check = load_json(name)
            if check.enabled:
                checks.append(check)
        except:
            print("Error while loading " + name)
            raise
    return checks


if 'CLAZY_NO_WERROR' in os.environ:
    del os.environ['CLAZY_NO_WERROR']

os.environ['CLAZY_CHECKS'] = ''

all_check_names = get_check_names()
all_checks = load_checks(all_check_names)
requested_check_names = args.check_names
requested_check_names = list(
    map(lambda x: x.strip("/\\"), requested_check_names))

for check_name in requested_check_names:
    if check_name not in all_check_names:
        print("Unknown check: " + check_name)
        sys.exit(-1)

if not requested_check_names:
    requested_check_names = all_check_names

requested_checks = list(filter(
    lambda check: check.name in requested_check_names and check.name not in _excluded_checks, all_checks))
requested_checks = list(filter(
    lambda check: check.minimum_clang_version <= CLANG_VERSION, requested_checks))

threads = []

if _dump_ast:
    for check in requested_checks:
        os.chdir(check.name)
        dump_ast(check)
        os.chdir("..")
else:
    cleanup_fixit_files(requested_checks)
    # Each list is a list of Test to be worked on by a thread
    list_of_chunks = [[] for _ in range(_num_threads)]
    i = _num_threads
    for check in requested_checks:
        for test in check.tests:
            i = (i + 1) % _num_threads
            list_of_chunks[i].append(test)

    for tests in list_of_chunks:
        if not tests:
            continue

        t = Thread(target=run_unit_tests, args=(tests,))
        t.start()
        threads.append(t)

for thread in threads:
    thread.join()

if not _no_fixits and not run_fixit_tests(requested_checks):
    _was_successful = False

if _was_successful:
    print("SUCCESS")
    sys.exit(0)
else:
    print("FAIL")
    sys.exit(-1)
