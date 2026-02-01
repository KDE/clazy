#!/usr/bin/env python3
import queue
import sys
import os
import re
import threading
import multiprocessing
import argparse
from threading import Thread

from testutils.checks import Check
from testutils import Args
from testutils.commands import dump_ast_command
from testutils.run_unittest import run_unit_tests
from testutils.run_fixit import run_fixit_tests
from testutils.os_utils import get_command_output, run_command
from testutils.checks import load_checks
from testutils.qtinstallation import qt_installation


# cd into the folder containing this script
os.chdir(os.path.realpath(os.path.dirname(sys.argv[0])))

def compiler_name():
    if 'CLAZY_CXX' in os.environ:
        return os.environ['CLAZY_CXX']  # so we can set clazy.bat instead
    return os.getenv('CLANGXX', 'clang')

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
args = Args(**vars(parser.parse_args()))
if args.only_standalone and args.no_standalone:
    print("Error: --only-standalone is incompatible with --no-standalone")
    sys.exit(1)
# -------------------------------------------------------------------------------
# Global variables

_num_threads = args.jobs
_lock = threading.Lock()
any_version_found = False
if 6 in args.qt_versions and qt_installation(6, args.verbose).int_version != 0:
    any_version_found = True
if 5 in args.qt_versions and qt_installation(5, args.verbose).int_version != 0:
    any_version_found = True
if not any_version_found:
    sys.exit(1)

_excluded_checks = args.exclude.split(',') if args.exclude is not None else []

# -------------------------------------------------------------------------------
# utility functions #2

version, success = get_command_output(compiler_name() + ' --version', args.verbose)
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

if args.verbose:
    print('Found clang version: ' + str(version))

CLANG_VERSION = int(version.replace('git', '').replace('.', ''))
args.clang_version = CLANG_VERSION


def get_check_names():
    return list(filter(lambda entry: os.path.isdir(entry), os.listdir(".")))

def cleanup_fixit_files(checks):
    for check in checks:
        filestodelete = list(filter(lambda entry: entry.endswith(
            '.fixed') or entry.endswith('.yaml'), os.listdir(check.name)))
        for f in filestodelete:
            os.remove(check.name + '/' + f)

def dump_ast(check: Check):
    for test in check.tests:
        for cppStandard in test.cppStandards:
            for version in test.qt_major_versions:
                if version == 6 and cppStandard == "c++14":
                    continue # Qt6 requires C++17
                ast_filename = test.filename + f"_{cppStandard}_{version}.ast"
                run_command(dump_ast_command(test, cppStandard, version) + " > " + ast_filename)
                print("Dumped AST to " + os.getcwd() + "/" + ast_filename)


if 'CLAZY_NO_WERROR' in os.environ:
    del os.environ['CLAZY_NO_WERROR']

os.environ['CLAZY_CHECKS'] = ''

all_check_names = get_check_names()
all_checks = load_checks(all_check_names, CLANG_VERSION)
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
finished_job_results: queue.Queue[bool] = queue.Queue()
if args.dump_ast:
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

        t = Thread(target=run_unit_tests, args=(tests, args, finished_job_results))
        t.start()
        threads.append(t)

for thread in threads:
    thread.join()

if not args.no_fixits and not run_fixit_tests(requested_checks, args):
    finished_job_results.put(False)

overall_success = all(finished_job_results.get() for _ in range(finished_job_results.qsize()))
if overall_success:
    print("SUCCESS")
    sys.exit(0)
else:
    print("FAIL")
    sys.exit(-1)
