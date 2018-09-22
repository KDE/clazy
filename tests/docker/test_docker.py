#!/usr/bin/env python2

import sys, os, json, argparse

JSON_CONFIG_FILENAME = os.path.dirname(sys.argv[0]) + '/conf.json'
MAKEFLAGS = "-j12"
BRANCH = 'master'
BUILD_SCRIPT = '/usr/bin/build-clazy.sh'

class DockerTest:
    def __init__(self, name):
        self.name = name
        self.prefix = '/opt/clazy'

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
    cmd = 'docker run -i -t %s sh %s %s %s %s' % (dockerTest.name, BUILD_SCRIPT, BRANCH, MAKEFLAGS, dockerTest.prefix)
    print cmd
    return os.system(cmd) == 0


dockerTests = read_json_config()


parser = argparse.ArgumentParser()
parser.add_argument("-b", "--branch")
parser.add_argument("docker_names", nargs='*', help="Names of the containers to run. Defaults to running all docker containers.")

args = parser.parse_args()

if args.branch is None:
    BRANCH = 'master'

results = {}
success = True
for test in dockerTests:
    if args.docker_names and test.name not in args.docker_names:
        continue

    results[test.name] = run_test(test)
    success = success and results[test.name]

if success:
    print "Success!"
else:
    for testname in results.keys():
        if not results[testname]:
            print "Test %s failed!" % testname

sys.exit(0 if success else 1)
