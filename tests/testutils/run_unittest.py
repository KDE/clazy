import io
import os
import queue
import subprocess
from pathlib import Path
from sys import platform as _platform
import platform

from . import Args
from .commands import clang_tidy_command, clazy_standalone_command, clazy_command
from .os_utils import run_command, compare_files
from .qtinstallation import qt_installation


def clazy_standalone_binary():
    if 'CLAZYSTANDALONE_CXX' in os.environ:  # in case we want to use "clazy.AppImage --standalone" instead
        return os.environ['CLAZYSTANDALONE_CXX']
    return 'clazy-standalone'


def is32Bit():
    return platform.architecture()[0] == '32bit'


def file_contains(filename, text):
    f = io.open(filename, 'r', encoding='utf-8')
    contents = f.read()
    f.close()
    return text in contents

def print_file(filename):
    f = open(filename, 'r')
    print(f.read())
    f.close()

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


def run_unit_test(test, is_standalone, is_tidy, cppStandard, qt_major_version, config: Args):
    if test.check.clazy_standalone_only and not is_standalone:
        return True

    qt = qt_installation(qt_major_version, config.verbose)
    if qt is None:
        return True  # silently skip

    if is_tidy and (test.isScript() or test.only_qt or test.qt_developer):
        print("Options not supported with clang-tidy")
        return True

    if config.verbose:
        print("Qt major versions required by the test: " + str(test.qt_major_versions))
        print("Currently considering Qt major version: " + str(qt_major_version))
        print("Qt versions required by the test: min=" + str(test.minimum_qt_version) + " max=" + str(test.maximum_qt_version))
        print("Qt int version: " + str(qt.int_version))
        print("Qt headers: " + qt.qmake_header_path)

    printableName = test.printableName(cppStandard, qt_major_version, is_standalone, is_tidy, False)

    if qt.int_version < test.minimum_qt_version or qt.int_version > test.maximum_qt_version or config.clang_version < test.minimum_clang_version:
        if config.verbose:
            print(f"Skipping {printableName} because required version is not available")
        return True

    if _platform in test.blacklist_platforms:
        if config.verbose:
            print(f"Skipping {printableName} because it is blacklisted for this platform")
        return True

    if not test.should_run_on_32bit and is32Bit():
        if config.verbose:
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
    if config.qt_namespaced and os.path.exists(expected_file_namespaced):
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
            clazy_standalone_command(test, cppStandard, qt, config)
    elif is_tidy:
        cmd_to_run = clang_tidy_command(test, cppStandard, qt, abs_filename, config)
    else:
        cmd_to_run = clazy_command(test, cppStandard, qt, abs_filename, config)

    if test.compare_everything:
        result_file = output_file

    must_fail = test.must_fail

    cmd_success = run_command(cmd_to_run, output_file, test.env, ignore_verbose_command=True, qt_namespaced=config.qt_namespaced, qt_replace_namespace=not qt_namespaced_separate_file)

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
    if not compare_files(test.expects_failure, expected_file, result_file, printableName, verbose=config.verbose):
        return False

    if test.has_fixits and not is_tidy and not config.qt_namespaced:
        # The normal tests succeeded, we can run the respective fixits then
        test.should_run_fixits_test = True

    return True


def run_unit_test_for_each_configuration(test, is_standalone, is_tidy, config: Args):
    """Runs unit tests for cpp standards and qt version combinations"""
    if test.check.clazy_standalone_only and not is_standalone:
        return True
    if config.qt_namespaced and test.skip_qtnamespaced:
        return True
    result = True
    for qt_major_version in test.qt_major_versions:
        for cppStandard in test.cppStandards:
            if cppStandard == "c++14" and qt_major_version == 6: # Qt6 requires C++17
                continue
            if cppStandard == "c++17" and qt_major_version == 5 and len(test.cppStandards) > 1: # valid combination but let's skip it unless it was the only specified standard
                continue
            result = result and run_unit_test(test, is_standalone, is_tidy, cppStandard, qt_major_version, config)
    return result

def run_unit_tests(tests, config: Args, finished_job_results: queue.Queue[bool]):
    result = True
    for test in tests:
        test_result = True
        if not config.only_standalone:
            test_result = run_unit_test_for_each_configuration(test, False, False, config)
            result = result and test_result

        if not config.no_standalone:
            test_result = test_result and run_unit_test_for_each_configuration(test, True, False, config)
            result = result and test_result

        if not config.no_clang_tidy:
            test_result = test_result and run_unit_test_for_each_configuration(test, False, True, config)
            result = result and test_result

        if not test_result:
            test.removeYamlFiles()

    finished_job_results.put(result)
