#!/usr/bin/env python2

import sys, os, json, argparse

JSON_CONFIG_FILENAME = os.path.dirname(sys.argv[0]) + '/conf.json'
MAKEFLAGS = "-j12"
BRANCH = 'master'
BUILD_SCRIPT = '/root/clazy/tests/docker/build-clazy.sh'

class DockerTest:
    def __init__(self, name, url):
        self.name = name
        self.url = url
        self.prefix = '/opt/clazy'
        self.llvm_root = ''

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
            if 'name' in test and 'url' in test:
                dockerTest = DockerTest(test['name'], test['url'])
                if 'prefix' in test:
                    dockerTest.prefix = test['prefix']
                if 'llvm_root' in test:
                    dockerTest.llvm_root = test['llvm_root']

                dockerTests.append(dockerTest)
    return dockerTests



def run_test(dockerTest):
    cmd = 'docker run -i -t %s sh %s %s %s %s %s' % (dockerTest.url, BUILD_SCRIPT, BRANCH, MAKEFLAGS, dockerTest.prefix dockerTest.llvm_root)
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
