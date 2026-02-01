# This is run sequentially, due to races. As clang-apply-replacements just applies all .yaml files it can find.
# We run a single clang-apply-replacements invocation, which changes all files in the tests/ directory.
import os
import shutil

from . import Args
from .os_utils import run_command, compare_files


def run_fixit_tests(requested_checks, config: Args):

    success = patch_yaml_files(requested_checks, is_standalone=False, no_standalone=config.no_standalone, only_standalone=config.only_standalone)
    success = patch_yaml_files(requested_checks, is_standalone=True, no_standalone=config.no_standalone, only_standalone=config.only_standalone) and success

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
                if not compare_fixit_results(test, is_standalone=False, no_standalone=config.no_standalone, only_standalone=config.only_standalone, verbose=config.verbose):
                    success = False
                    continue

                if not compare_fixit_results(test, is_standalone=True, no_standalone=config.no_standalone, only_standalone=config.only_standalone, verbose=config.verbose):
                    success = False
                    continue

    return success

def compare_fixit_results(test, is_standalone: bool, no_standalone: bool, only_standalone: bool, verbose: bool):

    if (is_standalone and no_standalone) or (not is_standalone and only_standalone):
        # Nothing to do
        return True

    # Check that the rewritten file is identical to the expected one
    if not compare_files(False, test.expectedFixedFilename(), test.fixedFilename(is_standalone), test.printableName("", 0, is_standalone, False, True), verbose=verbose):
        return False

    # Some fixed cpp files have an header that was also fixed. Compare it here too.
    possible_headerfile_expected = test.expectedFixedFilename().replace('.cpp', '.h')
    if os.path.exists(possible_headerfile_expected):
        possible_headerfile = test.fixedFilename(is_standalone).replace('.cpp', '.h')
        if not compare_files(False, possible_headerfile_expected, possible_headerfile, test.printableName("", 0, is_standalone, False, True).replace('.cpp', '.h'), verbose=verbose):
            return False

    return True


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


def patch_yaml_files(requested_checks, is_standalone: bool, no_standalone: bool, only_standalone: bool):
    if (is_standalone and no_standalone) or (not is_standalone and only_standalone):
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

def run_clang_apply_replacements(check):
    command = os.getenv('CLAZY_CLANG_APPLY_REPLACEMENTS', 'clang-apply-replacements')
    return run_command(command + ' ' + check.name)
