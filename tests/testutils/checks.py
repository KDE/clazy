import json
import os
from .test import Test

class Check:
    def __init__(self, name):
        self.name = name
        self.minimum_clang_version = 380  # clang 3.8.0
        self.minimum_qt_version = 500
        self.maximum_qt_version = 69999
        self.enabled = True
        self.clazy_standalone_only = False
        self.tests = []

def load_check_from_json(check_name: str, clang_version: int) -> Check:
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

    for key, value in decoded.items():
        if hasattr(check, key) and key != "tests":
            setattr(check, key, value)

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
                test.has_fixits = t['has_fixits'] and test.minimum_clang_version_for_fixits <= clang_version
            if 'expects_failure' in t:
                test.expects_failure = t['expects_failure']
            if 'skip_qtnamespaced' in t:
                test.skip_qtnamespaced = t['skip_qtnamespaced']
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


def load_checks(all_check_names, clang_version: int):
    checks = []
    for name in all_check_names:
        try:
            check = load_check_from_json(name, clang_version)
            if check.enabled:
                checks.append(check)
        except:
            print("Error while loading " + name)
            raise
    return checks
