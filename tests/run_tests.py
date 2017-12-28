#!/usr/bin/env python2

import sys, os, subprocess, string, re, json, threading, multiprocessing, argparse
from threading import Thread
from sys import platform as _platform

def isWindows():
    return _platform == 'win32'

class QtInstallation:
    def __init__(self):
        self.int_version = 000
        self.qmake_header_path = "/usr/include/qt/"

    def compiler_flags(self):
        return "-isystem " + self.qmake_header_path + ("" if isWindows() else " -fPIC")

class Test:
    def __init__(self, check):
        self.filename = ""
        self.minimum_qt_version = 500
        self.maximum_qt_version = 999
        self.minimum_clang_version = 380
        self.compare_everything = False
        self.isFixedFile = False
        self.link = False # If true we also call the linker
        self.check = check
        self.expects_failure = False
        self.qt_major_version = 5 # Tests use Qt 5 by default
        self.env = os.environ
        self.checks = []
        self.flags = ""
        self.must_fail = False
        self.blacklist_platforms = []
        self.qt4compat = False
        self.only_qt = False
        self.qt_developer = False

    def isScript(self):
        return self.filename.endswith(".sh")

    def setQtMajorVersion(self, major_version):
        if major_version == 4:
            self.qt_major_version = 4
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
            key_str = key.encode('ascii', 'ignore')
            self.env[key_str] = e[key].encode('ascii', 'ignore')

class Check:
    def __init__(self, name):
        self.name = name
        self.minimum_clang_version = 380 # clang 3.8.0
        self.minimum_qt_version = 500
        self.maximum_qt_version = 999
        self.enabled = True
        self.tests = []
#-------------------------------------------------------------------------------
# utility functions #1

def get_command_output(cmd, test_env = os.environ):
    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True, env=test_env)
    except subprocess.CalledProcessError, e:
        return e.output,False

    return output,True

def load_json(check_name):
    check = Check(check_name)
    filename = check_name + "/config.json"
    if not os.path.exists(filename):
        print "Error: " + filename + " not found"
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

    if 'blacklist_platforms' in decoded:
        check_blacklist_platforms = decoded['blacklist_platforms']

    if 'tests' in decoded:
        for t in decoded['tests']:
            test = Test(check)
            test.blacklist_platforms = check_blacklist_platforms
            test.filename = t['filename']

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

            if 'blacklist_platforms' in t:
                test.blacklist_platforms = t['blacklist_platforms']
            if 'compare_everything' in t:
                test.compare_everything = t['compare_everything']
            if 'isFixedFile' in t:
                test.isFixedFile = t['isFixedFile']
            if 'link' in t:
                test.link = t['link']
            if 'qt_major_version' in t:
                test.setQtMajorVersion(t['qt_major_version'])
            if 'env' in t:
                test.setEnv(t['env'])
            if 'checks' in t:
                test.checks = t['checks']
            if 'flags' in t:
                test.flags = t['flags']
            if 'must_fail' in t:
                test.must_fail = t['must_fail']
            if 'expects_failure' in t:
                test.expects_failure = t['expects_failure']
            if 'qt4compat' in t:
                test.qt4compat = t['qt4compat']
            if 'only_qt' in t:
                test.only_qt = t['only_qt']
            if 'qt_developer' in t:
                test.qt_developer = t['qt_developer']

            if not test.checks:
                test.checks.append(test.check.name)

            check.tests.append(test)
            if test.isFixedFile:
                fileToDelete = check_name + "/" + test.filename
                if os.path.exists(fileToDelete):
                    os.remove(fileToDelete)

    return check

def find_qt_installation(major_version, qmakes):
    installation = QtInstallation()

    for qmake in qmakes:
        qmake_version_str,success = get_command_output(qmake + " -query QT_VERSION")
        if success and qmake_version_str.startswith(str(major_version) + "."):
            qmake_header_path = get_command_output(qmake + " -query QT_INSTALL_HEADERS")[0].strip()
            if qmake_header_path:
                installation.qmake_header_path = qmake_header_path
                installation.int_version = int(qmake_version_str.replace(".", ""))
                if _verbose:
                    print "Found Qt " + str(installation.int_version) + " using qmake " + qmake
            break

    if installation.int_version == 0 and major_version >= 5: # Don't warn for missing Qt4 headers
        print "Error: Couldn't find a Qt" + str(major_version) + " installation"
    return installation

def libraryName():
    if _platform == 'win32':
        return 'ClangLazy.dll'
    elif _platform == 'darwin':
        return 'ClangLazy.dylib'
    else:
        return 'ClangLazy.so'

def link_flags():
    flags = "-lQt5Core -lQt5Gui -lQt5Widgets"
    if _platform.startswith('linux'):
        flags += " -lstdc++"
    return flags

def clazy_cpp_args():
    return "-Wno-unused-value -Qunused-arguments -std=c++14 "

def more_clazy_args():
    return " -Xclang -plugin-arg-clang-lazy -Xclang no-inplace-fixits " + clazy_cpp_args()

def clazy_standalone_command(test, qt):
    result = " -- " + clazy_cpp_args() + qt.compiler_flags() + " " + test.flags
    result = " -no-inplace-fixits -checks=" + string.join(test.checks, ',') + " " + result

    if not test.isFixedFile:
        result = " -enable-all-fixits " + result

    if test.qt4compat:
        result = " -qt4-compat " + result

    if test.only_qt:
        result = " -only-qt " + result

    if test.qt_developer:
        result = " -qt-developer " + result

    return result

def clazy_command(qt, test, filename):
    if test.isScript():
        return "./" + filename

    if 'CLAZY_CXX' in os.environ: # In case we want to use clazy.bat
        result = os.environ['CLAZY_CXX'] + more_clazy_args() + qt.compiler_flags()
    else:
        clang = os.getenv('CLANGXX', 'clang')
        result = clang + " -Xclang -load -Xclang " + libraryName() + " -Xclang -add-plugin -Xclang clang-lazy " + more_clazy_args() + qt.compiler_flags()

    if test.qt4compat:
        result = result + " -Xclang -plugin-arg-clang-lazy -Xclang qt4-compat "

    if test.only_qt:
        result = result + " -Xclang -plugin-arg-clang-lazy -Xclang only-qt "

    if test.qt_developer:
        result = result + " -Xclang -plugin-arg-clang-lazy -Xclang qt-developer "

    if test.link and _platform.startswith('linux'): # Linking on one platform is enough. Won't waste time on macOS and Windows.
        result = result + " " + link_flags()
    else:
        result = result + " -c "

    result = result + test.flags + " -Xclang -plugin-arg-clang-lazy -Xclang " + string.join(test.checks, ',') + " "
    if not test.isFixedFile: # When compiling the already fixed file disable fixit, we don't want to fix twice
        result += _enable_fixits_argument + " "
    result += filename

    return result

def dump_ast_command(test):
    return "clang -std=c++14 -fsyntax-only -Xclang -ast-dump -fno-color-diagnostics -c " + qt_installation(test.qt_major_version).compiler_flags() + " " + test.filename

def compiler_name():
    if 'CLAZY_CXX' in os.environ:
        return os.environ['CLAZY_CXX'] # so we can set clazy.bat instead
    return os.getenv('CLANGXX', 'clang')

#-------------------------------------------------------------------------------
# Get clang version
version,success = get_command_output(compiler_name() + ' --version')

match = re.search('clang version (.*?)[ -]', version)
try:
    version = match.group(1)
except:
    print "Could not determine clang version, is it in PATH?"
    sys.exit(-1)

CLANG_VERSION = int(version.replace('.', ''))

#-------------------------------------------------------------------------------
# Setup argparse

parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action='store_true')
parser.add_argument("--no-standalone", action='store_true', help="Don\'t run clazy-standalone")
parser.add_argument("--only-standalone", action='store_true', help='Only run clazy-standalone')
parser.add_argument("--dump-ast", action='store_true', help='Dump a unit-test AST to file')
parser.add_argument("check_names", nargs='*', help="The name of the check who's unit-tests will be run. Defaults to running all checks.")
args = parser.parse_args()

if args.only_standalone and args.no_standalone:
    print "Error: --only-standalone is incompatible with --no-standalone"
    sys.exit(1)

#-------------------------------------------------------------------------------
# Global variables

_enable_fixits_argument = "-Xclang -plugin-arg-clang-lazy -Xclang enable-all-fixits"
_dump_ast = args.dump_ast
_verbose = args.verbose
_no_standalone = args.no_standalone
_only_standalone = args.only_standalone
_num_threads = multiprocessing.cpu_count()
_lock = threading.Lock()
_was_successful = True
_qt5_installation = find_qt_installation(5, ["QT_SELECT=5 qmake", "qmake-qt5", "qmake"])
_qt4_installation = find_qt_installation(4, ["QT_SELECT=4 qmake", "qmake-qt4", "qmake"])
#-------------------------------------------------------------------------------
# utility functions #2

def qt_installation(major_version):
    if major_version == 5:
        return _qt5_installation
    elif major_version == 4:
        return _qt4_installation

    return None

def run_command(cmd, output_file = "", test_env = os.environ):
    lines,success = get_command_output(cmd, test_env)
    lines = lines.replace("std::_Container_base0", "std::_Vector_base") # Hack for Windows, we have std::_Vector_base in the expected data
    lines = lines.replace("std::__1::__vector_base_common", "std::_Vector_base") # Hack for macOS
    lines = lines.replace("std::_Vector_alloc", "std::_Vector_base")
    if not success and not output_file:
        print lines
        return False

    if _verbose:
        print "Running: " + cmd
        print "output_file=" + output_file

    lines = lines.replace('\r\n', '\n')
    if output_file:
        f = open(output_file, 'w')
        f.writelines(lines)
        f.close()
    else:
        print lines

    return success

def files_are_equal(file1, file2):
    try:
        f = open(file1, 'r')
        lines1 = f.readlines()
        f.close()

        f = open(file2, 'r')
        lines2 = f.readlines()
        f.close()

        return lines1 == lines2
    except:
        return False

def get_check_names():
    return filter(lambda entry: os.path.isdir(entry), os.listdir("."))

# Returns all files with .cpp_fixed extension. These were rewritten by clang.
def get_fixed_files():
    return filter(lambda entry: entry.endswith('.cpp_fixed.cpp'), os.listdir("."))

def print_differences(file1, file2):
    # Returns true if the the files are equal
    return run_command("diff -Naur {} {}".format(file1, file2))

def normalizedCwd():
    return os.getcwd().replace('\\', '/')

def extract_word(word, in_file, out_file):
    in_f = open(in_file, 'r')
    out_f = open(out_file, 'w')
    for line in in_f:
        if word in line:
            line = line.replace('\\', '/')
            line = line.replace(normalizedCwd() + '/', "") # clazy-standalone prints the complete cpp file path for some reason. Normalize it so it compares OK with the expected output.
            out_f.write(line)
    in_f.close()
    out_f.close()

def run_unit_test(test, is_standalone):
    qt = qt_installation(test.qt_major_version)

    if _verbose:
        print
        print "Qt version: " + str(qt.int_version)
        print "Qt headers: " + qt.qmake_header_path

    if qt.int_version < test.minimum_qt_version or qt.int_version > test.maximum_qt_version or CLANG_VERSION < test.minimum_clang_version:
        return True

    if _platform in test.blacklist_platforms:
        return True

    checkname = test.check.name
    filename = checkname + "/" + test.filename

    output_file = filename + ".out"
    result_file = filename + ".result"
    expected_file = filename + ".expected"

    if is_standalone and test.isScript():
        return True

    if is_standalone:
        cmd_to_run = "clazy-standalone " + filename + " " + clazy_standalone_command(test, qt)
    else:
        cmd_to_run = clazy_command(qt, test, filename)

    if test.compare_everything:
        result_file = output_file

    if test.isFixedFile:
        result_file = filename

    must_fail = test.must_fail

    cmd_success = run_command(cmd_to_run, output_file, test.env)

    if (not cmd_success and not must_fail) or (cmd_success and must_fail):
        print "[FAIL] " + checkname + " (Failed to build test. Check " + output_file + " for details)"
        print
        return False

    if not test.compare_everything and not test.isFixedFile:
        word_to_grep = "warning:" if not must_fail else "error:"
        extract_word(word_to_grep, output_file, result_file)

    printableName = checkname
    if len(test.check.tests) > 1:
        printableName += "/" + test.filename

    if is_standalone:
        printableName += " (standalone)"

    success = files_are_equal(expected_file, result_file)

    if test.expects_failure:
        if success:
            print "[XOK]   " + printableName
            return False
        else:
            print "[XFAIL] " + printableName
            print_differences(expected_file, result_file)
    else:
        if success:
            print "[OK]   " + printableName
        else:
            print "[FAIL] " + printableName
            print_differences(expected_file, result_file)
            return False

    return True

def run_unit_tests(tests):
    result = True
    for test in tests:
        if not _only_standalone:
            result = result and run_unit_test(test, False)

        if not _no_standalone:
            result = result and run_unit_test(test, True)

    global _was_successful, _lock
    with _lock:
        _was_successful = _was_successful and result

def dump_ast(check):
    for test in check.tests:
        ast_filename = test.filename + ".ast"
        run_command(dump_ast_command(test) + " > " + ast_filename)
        print "Dumped AST to " + os.getcwd() + "/" + ast_filename
#-------------------------------------------------------------------------------
def load_checks(all_check_names):
    checks = []
    for name in all_check_names:
        try:
            check = load_json(name)
            if check.enabled:
                checks.append(check)
        except:
            print "Error while loading " + name
            raise
            sys.exit(-1)
    return checks
#-------------------------------------------------------------------------------
# main

if 'CLAZY_NO_WERROR' in os.environ:
    del os.environ['CLAZY_NO_WERROR']

os.environ['CLAZY_CHECKS'] = ''

all_check_names = get_check_names()
all_checks = load_checks(all_check_names)
requested_check_names = args.check_names
requested_check_names = map(lambda x: x.strip("/\\"), requested_check_names)

for check_name in requested_check_names:
    if check_name not in all_check_names:
        print "Unknown check: " + check_name
        print
        sys.exit(-1)

if not requested_check_names:
    requested_check_names = all_check_names

requested_checks = filter(lambda check: check.name in requested_check_names, all_checks)
requested_checks = filter(lambda check: check.minimum_clang_version <= CLANG_VERSION, requested_checks)

threads = []

if _dump_ast:
    for check in requested_checks:
        os.chdir(check.name)
        dump_ast(check)
        os.chdir("..")
else:
    list_of_chunks = [[] for x in range(_num_threads)]  # Each list is a list of Test to be worked on by a thread
    i = _num_threads
    for check in requested_checks:
        for test in check.tests:
            if not test.isFixedFile:
                i = (i + 1) % _num_threads

            list_of_chunks[i].append(test)

    for tests in list_of_chunks:
        if not tests:
            continue;

        t = Thread(target=run_unit_tests, args=(tests,))
        t.start()
        threads.append(t)

for thread in threads:
    thread.join()

sys.exit(0 if _was_successful else -1)
