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

    tests = decoded['tests'] # OK to throe error if not exist
    del decoded["tests"]
    for key, value in decoded.items():
        if hasattr(check, key):
            setattr(check, key, value)
    if 'blacklist_platforms' in decoded:
        check_blacklist_platforms = decoded['blacklist_platforms']

    for t in tests:
        test = Test(check)
        test.blacklist_platforms = check_blacklist_platforms
        test.minimum_qt_version = check.minimum_qt_version
        test.minimum_clang_version = check.minimum_clang_version
        test.maximum_qt_version = check.maximum_qt_version

        for key, value in t.items():
            if hasattr(test, key):
                setattr(test, key, value)

        test.has_fixits = test.has_fixits and test.minimum_clang_version_for_fixits <= clang_version
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
