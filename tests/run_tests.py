#!/usr/bin/python2

import sys, os, subprocess, string, re

# Remove when we refactor unit-tests and allow to pass custom options
os.environ["CLAZY_EXTRA_OPTIONS"] = "qstring-arg-fillChar-overloads"

#-------------------------------------------------------------------------------
# utility functions #1

def get_command_output(cmd):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    if p.wait() != 0:
        return ""
    return p.communicate()[0];

#-------------------------------------------------------------------------------
# Detect Qt include path
# We try in order:
#   QT_SELECT=5 qmake
#   qmake-qt5
#   qmake
QMAKE_HEADERS = ""
qmakes = ["QT_SELECT=5 qmake", "qmake-qt5", "qmake"]
for qmake in qmakes:
    QMAKE_VERSION = get_command_output(qmake + " -query QT_VERSION")
    if QMAKE_VERSION.startswith("5."):
        QMAKE_HEADERS = get_command_output(qmake + " -query QT_INSTALL_HEADERS").strip()
        break

QMAKE_INT_VERSION = int(QMAKE_VERSION.replace(".", ""))

if not QMAKE_HEADERS:
    # Change here if can't find with qmake
    QMAKE_HEADERS = "/usr/include/qt/"

QT_FLAGS = "-isystem " + QMAKE_HEADERS + " -fPIC"

#-------------------------------------------------------------------------------
# Get clang version
version = get_command_output('clang --version')

match = re.search('clang version (.*?)[ -]', version)
version = match.group(1)

CLANG_VERSION = int(version.replace('.', ''))

#-------------------------------------------------------------------------------
# Global variables

_compiler_comand = "clang++ -std=c++11 -Wno-unused-value -Qunused-arguments -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy -Xclang -plugin-arg-clang-lazy -Xclang no-inplace-fixits -Xclang -plugin-arg-clang-lazy -Xclang enable-all-fixits " + QT_FLAGS
_link_flags = "-lQt5Core -lQt5Gui -lQt5Widgets"
_dump_ast_command = "clang++ -std=c++11 -fsyntax-only -Xclang -ast-dump -fno-color-diagnostics -c *.cpp " + QT_FLAGS
_help_command = "echo | clang++ -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy -Xclang -plugin-arg-clang-lazy -Xclang help -c -xc -"
_dump_ast = "--dump-ast" in sys.argv
_verbose = "--verbose" in sys.argv
_help = "--help" in sys.argv
_only_checks = "--only-checks" in sys.argv # If set, the tests for the compiler itself aren't run
_qtVersionLowerThan55 = QMAKE_INT_VERSION < 550
_qtVersionLowerThan53 = QMAKE_INT_VERSION < 530
_clangVersionLowerThan37 = CLANG_VERSION < 370
#-------------------------------------------------------------------------------
# utility functions #2

def run_command(cmd):
    if os.system(cmd) != 0:
        return False
    return True

def print_usage():
    print "Usage for " + sys.argv[0].strip("./") + ":\n"
    print "    " + sys.argv[0] + " [--help] [--dump-ast] [--only-checks] [check1,check2,check3]"
    print
    print "    Without any check supplied, all checks will be run."
    print "    --dump-ast is provided for debugging purposes.\n"
    print "Help for clang plugin:"
    print
    run_command(_help_command)

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

def get_check_list():
    return filter(lambda entry: os.path.isdir(entry), os.listdir("."))

# Returns all files with .cpp_fixed extension. These were rewritten by clang.
def get_fixed_files():
    return filter(lambda entry: entry.endswith('.cpp_fixed.cpp'), os.listdir("."))

def get_sh_files():
    return filter(lambda entry: entry.endswith('.sh'), os.listdir("."))

def print_differences(file1, file2):
    return run_command("diff -Naur {} {}".format(file1, file2))

def extract_word(word, in_file, out_file):
    in_f = open(in_file, 'r')
    out_f = open(out_file, 'w')
    for line in in_f:
        if word in line:
            out_f.write(line)
    in_f.close()
    out_f.close()

def cleanup_fixed_files():
    fixed_files = get_fixed_files()
    for fixed_file in fixed_files:
        os.remove(fixed_file)

def run_check_unit_tests(check):
    cmd = ""
    do_link_step = check in ['old-style-connect'] # Hardcoded for now

    if do_link_step:
        cmd += _compiler_comand + " " + _link_flags
    else:
        cmd += _compiler_comand + " -c "

    clazy_cmd = cmd + " -Xclang -plugin-arg-clang-lazy -Xclang " + check + " *.cpp"
    if _verbose:
        print "Running: " + clazy_cmd

    cleanup_fixed_files()

    if not run_command(clazy_cmd + " > all_files.compile_output 2> all_files.compile_output"):
        print "[FAIL] " + check + " (Failed to build test. Check " + check + "/all_files.compile_output for details)"
        print
        return False

    extract_word("warning:", "all_files.compile_output", "test.output")

    result = True

    if files_are_equal("test.expected", "test.output"):
        print "[OK]   " + check
    else:
        print "[FAIL] " + check
        if not print_differences("test.expected", "test.output"):
            result = False

    # If fixits were applied, test they were correctly applied
    fixed_files = string.join(get_fixed_files(), ' ')
    if fixed_files:
        output_file = "all_files_fixed.compile_output"
        if run_command(cmd + " " + fixed_files + " > " + output_file + " 2> " + output_file):
            print "   [OK]   fixed file  "
        else:
            print "   [FAIL] fixed file (Failed to build test. Check " + check + "/" + output_file + " for details) Files were: " + fixed_files
            print
            result = False

    return result

def run_core_tests():
    scripts = get_sh_files()
    for script in scripts:
        expected_file = script + ".expected"
        output_file = script + ".output"
        if not  run_command("./" + script + " > " + output_file + " 2>&1"):
            return False
        if files_are_equal(expected_file, output_file):
            print "[OK]   clazy"
            return True
        else:
            print "[FAIL] clazy"
            print_differences(expected_file, output_file)
            return False

    return True

def dump_ast(check):
    run_command(_dump_ast_command + " > dump.ast")
#-------------------------------------------------------------------------------
# main

if _help:
    print_usage()
    sys.exit(0)

args = sys.argv[1:]

switches = ["--verbose", "--dump-ast", "--help", "--only-checks"]

if _dump_ast:
    del(args[args.index("--dump-ast")])


all_checks = get_check_list()
requested_checks = filter(lambda x: x not in switches, args)
requested_checks = map(lambda x: x.strip("/"), requested_checks)

for check in requested_checks:
    if check not in all_checks:
        print "Unknown check: " + check
        print
        sys.exit(-1)

if not requested_checks:
    requested_checks = all_checks
    if _qtVersionLowerThan55:
        # These checks don't pass on Qt 5.4 due to missing API
        requested_checks = filter(lambda x: x not in ["old-style-connect", "detaching-temporary"] , requested_checks)
    if _qtVersionLowerThan53:
        # 1% os reserve-candidates tests don't pass on 5.2.1, not worth wasting time on
        requested_checks = filter(lambda x: x not in ["reserve-candidates"] , requested_checks)

    if _clangVersionLowerThan37:
        requested_checks = filter(lambda x: x not in ["qstring-uneeded-heap-allocations", "clazy"] , requested_checks)

for check in requested_checks:
    os.chdir(check)
    if _dump_ast:
        dump_ast(check)
    else:
        if check == "clazy":
            if not _only_checks and not run_core_tests():
                exit(-1)
        elif not run_check_unit_tests(check):
            exit(-1)

    os.chdir("..")
