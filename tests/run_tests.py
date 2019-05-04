#!/usr/bin/env python2

import sys, os, subprocess, string, re, json, threading, multiprocessing, argparse
import shutil
from threading import Thread
from sys import platform as _platform

_verbose = False

def isWindows():
    return _platform == 'win32'

class QtInstallation:
    def __init__(self):
        self.int_version = 000
        self.qmake_header_path = "/usr/include/qt/"
        self.qmake_lib_path = "/usr/lib"

    def compiler_flags(self):
        return "-isystem " + self.qmake_header_path + ("" if isWindows() else " -fPIC") + " -L " + self.qmake_lib_path

class Test:
    def __init__(self, check):
        self.filenames = []
        self.minimum_qt_version = 500
        self.maximum_qt_version = 59999
        self.minimum_clang_version = 380
        self.compare_everything = False
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
        self.header_filter = ""
        self.ignore_dirs = ""
        self.has_fixits = False

    def filename(self):
        if len(self.filenames) == 1:
            return self.filenames[0]
        return ""

    def relativeFilename(self):
        return self.check.name + "/" + self.filename()

    def yamlFilename(self):
        # In case clazy-standalone generates a yaml file with fixits, this is what it will be called
        return self.relativeFilename() + ".yaml"

    def fixedFilename(self):
        return self.relativeFilename() + ".fixed"

    def expectedFixedFilename(self):
        return self.relativeFilename() + ".fixed.expected"

    def isScript(self):
        return self.filename().endswith(".sh")

    def dir(self):
        return self.check.name

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
        self.maximum_qt_version = 59999
        self.enabled = True
        self.clazy_standalone_only = False
        self.tests = []
#-------------------------------------------------------------------------------
# utility functions #1

def get_command_output(cmd, test_env = os.environ):
    try:
        if _verbose:
            print cmd
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True, env=test_env)
    except subprocess.CalledProcessError, e:
        return e.output,False

    return output,True

def load_json(check_name):
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

            if 'blacklist_platforms' in t:
                test.blacklist_platforms = t['blacklist_platforms']
            if 'compare_everything' in t:
                test.compare_everything = t['compare_everything']
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
            if 'has_fixits' in t:
                test.has_fixits = t['has_fixits']
            if 'expects_failure' in t:
                test.expects_failure = t['expects_failure']
            if 'qt4compat' in t:
                test.qt4compat = t['qt4compat']
            if 'only_qt' in t:
                test.only_qt = t['only_qt']
            if 'qt_developer' in t:
                test.qt_developer = t['qt_developer']
            if 'header_filter' in t:
                test.header_filter = t['header_filter']
            if 'ignore_dirs' in t:
                test.ignore_dirs = t['ignore_dirs']

            if not test.checks:
                test.checks.append(test.check.name)

            check.tests.append(test)

    return check

def find_qt_installation(major_version, qmakes):
    installation = QtInstallation()

    for qmake in qmakes:
        qmake_version_str,success = get_command_output(qmake + " -query QT_VERSION")
        if success and qmake_version_str.startswith(str(major_version) + "."):
            qmake_header_path = get_command_output(qmake + " -query QT_INSTALL_HEADERS")[0].strip()
            qmake_lib_path = get_command_output(qmake + " -query QT_INSTALL_LIBS")[0].strip()
            if qmake_header_path:
                installation.qmake_header_path = qmake_header_path
                if qmake_lib_path:
                    installation.qmake_lib_path = qmake_lib_path
                ver = qmake_version_str.split('.')
                installation.int_version = int(ver[0]) * 10000 + int(ver[1]) * 100 + int(ver[2])
                if _verbose:
                    print "Found Qt " + str(installation.int_version) + " using qmake " + qmake
            break

    if installation.int_version == 0 and major_version >= 5: # Don't warn for missing Qt4 headers
        print "Error: Couldn't find a Qt" + str(major_version) + " installation"
    return installation

def libraryName():
    if _platform == 'win32':
        return 'ClazyPlugin.dll'
    elif _platform == 'darwin':
        return 'ClazyPlugin.dylib'
    else:
        return 'ClazyPlugin.so'

def link_flags():
    flags = "-lQt5Core -lQt5Gui -lQt5Widgets"
    if _platform.startswith('linux'):
        flags += " -lstdc++"
    return flags

def clazy_cpp_args():
    return "-Wno-unused-value -Qunused-arguments -std=c++14 "

def more_clazy_args():
    return " " + clazy_cpp_args()

def clazy_standalone_binary():
    if 'CLAZYSTANDALONE_CXX' in os.environ: # in case we want to use "clazy.AppImage --standalone" instead
        return os.environ['CLAZYSTANDALONE_CXX']
    return 'clazy-standalone'

def clazy_standalone_command(test, qt):
    result = " -- " + clazy_cpp_args() + qt.compiler_flags() + " " + test.flags
    result = " -checks=" + string.join(test.checks, ',') + " " + result

    if test.has_fixits:
        result = " -enable-all-fixits -export-fixes=" + test.yamlFilename() + result

    if test.qt4compat:
        result = " -qt4-compat " + result

    if test.only_qt:
        result = " -only-qt " + result

    if test.qt_developer:
        result = " -qt-developer " + result

    if test.header_filter:
        result = " -header-filter " + test.header_filter + " " + result

    if test.ignore_dirs:
        result = " -ignore-dirs " + test.ignore_dirs + " " + result

    return result

def clazy_command(qt, test, filename):
    if test.isScript():
        return "./" + filename

    if 'CLAZY_CXX' in os.environ: # In case we want to use clazy.bat
        result = os.environ['CLAZY_CXX'] + more_clazy_args() + qt.compiler_flags()
    else:
        clang = os.getenv('CLANGXX', 'clang')
        result = clang + " -Xclang -load -Xclang " + libraryName() + " -Xclang -add-plugin -Xclang clazy " + more_clazy_args() + qt.compiler_flags()

    if test.qt4compat:
        result = result + " -Xclang -plugin-arg-clazy -Xclang qt4-compat "

    if test.only_qt:
        result = result + " -Xclang -plugin-arg-clazy -Xclang only-qt "

    if test.qt_developer:
        result = result + " -Xclang -plugin-arg-clazy -Xclang qt-developer "

    if test.link and _platform.startswith('linux'): # Linking on one platform is enough. Won't waste time on macOS and Windows.
        result = result + " " + link_flags()
    else:
        result = result + " -c "

    result = result + test.flags + " -Xclang -plugin-arg-clazy -Xclang " + string.join(test.checks, ',') + " "
    result += _enable_fixits_argument + " "
    result += filename

    return result

def dump_ast_command(test):
    return "clang -std=c++14 -fsyntax-only -Xclang -ast-dump -fno-color-diagnostics -c " + qt_installation(test.qt_major_version).compiler_flags() + " " + test.flags + " " + test.filename()

def compiler_name():
    if 'CLAZY_CXX' in os.environ:
        return os.environ['CLAZY_CXX'] # so we can set clazy.bat instead
    return os.getenv('CLANGXX', 'clang')

#-------------------------------------------------------------------------------
# Setup argparse

parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action='store_true')
parser.add_argument("--no-standalone", action='store_true', help="Don\'t run clazy-standalone")
parser.add_argument("--only-standalone", action='store_true', help='Only run clazy-standalone')
parser.add_argument("--dump-ast", action='store_true', help='Dump a unit-test AST to file')
parser.add_argument("--exclude", help='Comma separated list of checks to ignore')
parser.add_argument("check_names", nargs='*', help="The name of the check who's unit-tests will be run. Defaults to running all checks.")
args = parser.parse_args()

if args.only_standalone and args.no_standalone:
    print "Error: --only-standalone is incompatible with --no-standalone"
    sys.exit(1)

#-------------------------------------------------------------------------------
# Global variables

_enable_fixits_argument = "-Xclang -plugin-arg-clazy -Xclang enable-all-fixits"
_dump_ast = args.dump_ast
_verbose = args.verbose
_no_standalone = args.no_standalone
_only_standalone = args.only_standalone
_num_threads = multiprocessing.cpu_count()
_lock = threading.Lock()
_was_successful = True
_qt5_installation = find_qt_installation(5, ["QT_SELECT=5 qmake", "qmake-qt5", "qmake"])
_qt4_installation = find_qt_installation(4, ["QT_SELECT=4 qmake", "qmake-qt4", "qmake"])
_excluded_checks = args.exclude.split(',') if args.exclude is not None else []

#-------------------------------------------------------------------------------
# utility functions #2

version,success = get_command_output(compiler_name() + ' --version')

match = re.search('clang version (.*?)[ -]', version)
try:
    version = match.group(1)
except:
    # Now try the Clazy.AppImage way
    match = re.search('clang version: (.*)', version)

    try:
        version = match.group(1)
    except:
        print "Could not determine clang version, is it in PATH?"
        sys.exit(-1)

if _verbose:
    print 'Found clang version: ' + str(version)

CLANG_VERSION = int(version.replace('.', ''))

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

def compare_files(expected_file, result_file, message):
    success = files_are_equal(expected_file, result_file)

    if test.expects_failure:
        if success:
            print "[XOK]   " + message
            return False
        else:
            print "[XFAIL] " + message
            print_differences(expected_file, result_file)
            return True
    else:
        if success:
            print "[OK]   " + message
            return True
        else:
            print "[FAIL] " + message
            print_differences(expected_file, result_file)
            return False

def get_check_names():
    return filter(lambda entry: os.path.isdir(entry), os.listdir("."))

# The yaml file references the test file in our git repo, but we don't want
# to rewrite that one, as we would need to discard git changes afterwards,
# so patch the yaml file and add a ".fixed" suffix to those files
def patch_fixit_yaml_file(test):

    f = open(test.yamlFilename(), 'r')
    lines = f.readlines()
    f.close()
    f = open(test.yamlFilename(), 'w')

    for line in lines:
        stripped = line.strip()
        if stripped.startswith('MainSourceFile') or stripped.startswith("FilePath") or stripped.startswith("- FilePath"):
            line = line.replace(test.relativeFilename(), test.fixedFilename())
        f.write(line)
    f.close()

    shutil.copyfile(test.relativeFilename(), test.fixedFilename())

    return True

def run_clang_apply_replacements(test):
    result = run_command('clang-apply-replacements ' + test.check.name)
    return result

def cleanup_fixit_files():
    yamlfiles = filter(lambda entry: entry.endswith('.yaml'), os.listdir('.'))
    fixedfiles = filter(lambda entry: entry.endswith('.fixed'), os.listdir('.'))
    for f in yamlfiles:
        os.remove(f)
    for f in fixedfiles:
        os.remove(f)

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

def print_file(filename):
    f = open(filename, 'r')
    print f.read()
    f.close()

def file_contains(filename, text):
    f = open(filename, 'r')
    contents = f.read()
    f.close()
    return text in contents

def run_unit_test(test, is_standalone):
    if test.check.clazy_standalone_only and not is_standalone:
        return True

    qt = qt_installation(test.qt_major_version)

    if _verbose:
        print
        print "Qt version: " + str(qt.int_version)
        print "Qt headers: " + qt.qmake_header_path

    if qt.int_version < test.minimum_qt_version or qt.int_version > test.maximum_qt_version or CLANG_VERSION < test.minimum_clang_version:
        if (_verbose):
            print "Skipping " + test.check.name + " because required version is not available"
        return True

    if _platform in test.blacklist_platforms:
        if (_verbose):
            print "Skipping " + test.check.name + " because it is blacklisted for this platform"
        return True

    checkname = test.check.name
    filename = checkname + "/" + test.filename()

    output_file = filename + ".out"
    result_file = filename + ".result"
    expected_file = filename + ".expected"

    if is_standalone and test.isScript():
        return True

    if is_standalone:
        cmd_to_run = clazy_standalone_binary() + " " + filename + " " + clazy_standalone_command(test, qt)
    else:
        cmd_to_run = clazy_command(qt, test, filename)

    if test.compare_everything:
        result_file = output_file

    must_fail = test.must_fail

    cmd_success = run_command(cmd_to_run, output_file, test.env)

    if file_contains(output_file, 'Invalid check: '):
        return True

    if (not cmd_success and not must_fail) or (cmd_success and must_fail):
        print "[FAIL] " + checkname + " (Failed to build test. Check " + output_file + " for details)"
        print "-------------------"
        print "Contents of %s:" % output_file
        print_file(output_file)
        print "-------------------"
        print
        return False

    if not test.compare_everything:
        word_to_grep = "warning:" if not must_fail else "error:"
        extract_word(word_to_grep, output_file, result_file)

    printableName = checkname
    if len(test.check.tests) > 1:
        printableName += "/" + test.filename()

    if is_standalone:
        if test.has_fixits:
            printableNameFixits = printableName + " (standalone, fixits)"
        printableName += " (standalone)"

    # Check that it printed the expected warnings
    if not compare_files(expected_file, result_file, printableName):
        return False

    # Test passed, let's run fixits, if any
    if is_standalone and test.has_fixits:
        if not os.path.exists(test.yamlFilename()):
            print "[FAIL] " + test.yamlFilename() + " is missing!!"
            return False

        if not patch_fixit_yaml_file(test):
            print "[FAIL] Could not patch " + test.yamlFilename()
            return False
        if not run_clang_apply_replacements(test):
            print "[FAIL] Error applying fixits from " + test.yamlFilename()
            return False

        # Check that the rewritten file is identical to the expected one
        if not compare_files(test.expectedFixedFilename(), test.fixedFilename(), printableNameFixits):
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
        ast_filename = test.filename() + ".ast"
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

requested_checks = filter(lambda check: check.name in requested_check_names and check.name not in _excluded_checks, all_checks)
requested_checks = filter(lambda check: check.minimum_clang_version <= CLANG_VERSION, requested_checks)

threads = []

if _dump_ast:
    for check in requested_checks:
        os.chdir(check.name)
        dump_ast(check)
        os.chdir("..")
else:
    cleanup_fixit_files()
    list_of_chunks = [[] for x in range(_num_threads)]  # Each list is a list of Test to be worked on by a thread
    i = _num_threads
    for check in requested_checks:
        for test in check.tests:
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

if _was_successful:
    print "SUCCESS"
    sys.exit(0)
else:
    print "FAIL"
    sys.exit(-1)
