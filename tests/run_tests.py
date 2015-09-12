#!/usr/bin/python2

import sys, os, subprocess

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
#   qmake
#   QT_SELECT=5 qmake
#   qmake-qt5
QMAKE_HEADERS = ""
qmakes = ["qmake", "QT_SELECT=5 qmake", "qmake-qt5"]
for qmake in qmakes:
    QMAKE_VERSION = get_command_output(qmake + " -query QT_VERSION")
    if QMAKE_VERSION.startswith("5."):
        QMAKE_HEADERS = get_command_output(qmake + " -query QT_INSTALL_HEADERS").strip()
        break

if not QMAKE_HEADERS:
    # Change here if can't find with qmake
    QMAKE_HEADERS = "/usr/include/qt/"

QT_FLAGS = "-I " + QMAKE_HEADERS + " -fPIC"

#-------------------------------------------------------------------------------
# Global variables

_compiler_comand = "clang++ -std=c++11 -Wno-unused-value -Qunused-arguments -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy -Xclang -plugin-arg-clang-lazy -Xclang no-inplace-fixits -Xclang -plugin-arg-clang-lazy -Xclang enable-all-fixits -c " + QT_FLAGS
_dump_ast_command = "clang++ -std=c++11 -fsyntax-only -Xclang -ast-dump -fno-color-diagnostics -c *.cpp " + QT_FLAGS
_help_command = "clang++ -Xclang -load -Xclang ClangLazy.so -Xclang -add-plugin -Xclang clang-lazy -Xclang -plugin-arg-clang-lazy -Xclang help -c empty.cpp"
_dump_ast = "--dump-ast" in sys.argv
_verbose = "--verbose" in sys.argv
_help = "--help" in sys.argv

#-------------------------------------------------------------------------------
# utility functions #2

def run_command(cmd):
    if os.system(cmd) != 0:
        return False
    return True

def print_usage():
    print "Usage for " + sys.argv[0].strip("./") + ":\n"
    print "    " + sys.argv[0] + " [--help] [--dump-ast] [check1,check2,check3]"
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
    cmd = _compiler_comand + " -Xclang -plugin-arg-clang-lazy -Xclang " + check + " *.cpp "

    if _verbose:
        print "Running: " + cmd

    cleanup_fixed_files()

    if not run_command(cmd + " > all_files.compile_output 2> all_files.compile_output"):
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
    fixed_files = get_fixed_files()
    for fixed_file in fixed_files:
        output_file = fixed_file + ".compile_output"
        if run_command(_compiler_comand + " " + fixed_file + " > " + output_file + " 2> " + output_file):
            print "   [OK]   " + fixed_file
        else:
            print "   [FAIL] " + fixed_file + " (Failed to build test. Check " + check + "/" + output_file + " for details)"
            print
            result = False

    return result

def dump_ast(check):
    run_command(_dump_ast_command + " > dump.ast")
#-------------------------------------------------------------------------------
# main

if _help:
    print_usage()
    sys.exit(0)

args = sys.argv[1:]

switches = ["--verbose", "--dump-ast", "--help"]

if _dump_ast:
    del(args[args.index("--dump-ast")])


all_checks = get_check_list()
requested_checks = filter(lambda x: x not in switches, args)
requested_checks = map(lambda x: x.strip("/"), args)

for check in requested_checks:
    if check not in all_checks:
        print "Unknown check: " + check
        print
        sys.exit(-1)

if not requested_checks:
    requested_checks = all_checks

for check in requested_checks:
    os.chdir(check)
    if _dump_ast:
        dump_ast(check)
    else:
        run_check_unit_tests(check)

    os.chdir("..")
