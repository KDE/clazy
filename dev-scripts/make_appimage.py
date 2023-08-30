#!/usr/bin/env python


#  This file is part of the clazy static checker.

#  SPDX-FileCopyrightText: 2018 Klaralvdalens Datakonsult AB a KDAB Group company info@kdab.com

#  SPDX-License-Identifier: LGPL-2.0-or-later


# This is my quick and dirty script to generate clazy app images on each release
# Requires iamsergio/clazy-centos68 docker image to work

import sys, os

CLAZY_SHA1 = ''
WORK_FOLDER = '/tmp/clazy_work/'
DOCKER_IMAGE = 'iamsergio/clazy-centos68'
DEST_FILE = WORK_FOLDER + '/Clazy-x86_64.AppImage'

def print_usage():
    print(sys.argv[0] + ' <clazy sha1>')

def run_command(cmd, abort_on_error = True):
    print(cmd)
    success = (os.system(cmd) == 0)
    if abort_on_error and not success:
        sys.exit(1)

    return success

def prepare_folder():
    run_command('rm -rf ' + WORK_FOLDER)
    os.mkdir(WORK_FOLDER)

def make_appimage_in_docker():
    cmd = 'docker run -i -t -v %s:%s %s %s' % (WORK_FOLDER, WORK_FOLDER, DOCKER_IMAGE, 'bash -c "cd /clazy/ && git pull && /clazy/dev-scripts/docker/make_appimage.sh %s %s"' % (CLAZY_SHA1, str(os.getuid())))
    if not run_command(cmd):
        print('Error running docker. Make sure docker is running and that you have ' + DOCKER_IMAGE)

    os.environ['ARCH'] = 'x86_64'
    if not run_command('appimagetool-x86_64.AppImage %s/clazy.AppDir/ %s' % (WORK_FOLDER, DEST_FILE)):
        return False

    return True


def clazy_source_directory():
    return os.path.dirname(os.path.realpath(__file__)) + '/../'

def run_tests():
    os.chdir(clazy_source_directory() + '/tests/')
    os.environ['CLAZY_CXX'] = '/tmp/clazy_work//Clazy-x86_64.AppImage'
    os.environ['CLAZYSTANDALONE_CXX'] = '/tmp/clazy_work//Clazy-x86_64.AppImage --standalone'
    return run_command("./run_tests.py --verbose")


if len(sys.argv) != 2:
    print_usage();
    sys.exit(1)


CLAZY_SHA1 = sys.argv[1]

prepare_folder()

if not make_appimage_in_docker():
    sys.exit(1)

if not run_tests():
    sys.exit(1)

print('')
run_command('sha1sum ' + DEST_FILE)
run_command('sha256sum ' + DEST_FILE)

sign_script = os.getenv('CLAZY_SIGN_SCRIPT', '')

if sign_script:
    os.chdir(WORK_FOLDER)
    if not run_command(sign_script + ' ' + DEST_FILE):
        print('Error signing file')
        sys.exit(1)

print('')
print('Success: ' + DEST_FILE)
