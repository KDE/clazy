#!/usr/bin/env python2

import sys, os, json

JSON_CONFIG_FILENAME = os.path.dirname(sys.argv[0]) + '/conf.json'
MAKEFLAGS = "-j12"
BRANCH = 'master'
BUILD_SCRIPT = '/usr/bin/build-clazy.sh'

class DockerTest:
    def __init__(self, name):
        self.name = name


def read_json_config():
    dockerTests = []

    if not os.path.exists(JSON_CONFIG_FILENAME):
        print "File doesn't exist %s" % (JSON_CONFIG_FILENAME)
        return []

    f = open(JSON_CONFIG_FILENAME, 'r')
    contents = f.read()
    f.close()
    decoded = json.loads(contents)
    if 'tests' in decoded:
        tests = decoded['tests']
        for test in tests:
            if 'name' in test:
                dockerTest = DockerTest(test['name'])
                dockerTests.append(dockerTest)
    return dockerTests



def run_test(dockerTest):
    cmd = 'docker run -i -t %s sh %s %s %s' % (dockerTest.name, BUILD_SCRIPT, BRANCH, MAKEFLAGS)
    print cmd
    return os.system(cmd) == 0


dockerTests = read_json_config()

if len(sys.argv) > 1:
    BRANCH = sys.argv[1]


results = {}
success = True
for test in dockerTests:
    results[test.name] = run_test(test)
    success = success and results[test.name]

if success:
    print "Success!"
else:
    for testname in results.keys():
        if not results[testname]:
            print "Test %s failed!" % testname

sys.exit(0 if success else 1)
