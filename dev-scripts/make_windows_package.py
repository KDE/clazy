#!/usr/bin/env python

import sys, os
from shutil import copyfile

#--------------------------------------------------
# Change here:
CLAZY_VERSION = '1.4'
CANDIDATE_SHA1 = 'v1.4_windows_binaries'
PACKAGE_DIR = '/c/d/clazy-msvc-package/'
PACKAGE_DIR_WIN = 'c:\\d\\clazy-msvc-package\\'
#--------------------------------------------------

CLAZY_REPO_URL = "https://github.com/KDE/clazy.git"
MSVC_VERSION = os.getenv('MSVC_VERSION', '')
LLVM_INSTALL_DIR = os.getenv('LLVM_INSTALL_DIR', '')
CLAZY_WORK_DIR = 'work' + MSVC_VERSION
CLAZY_ZIP_WITHOUT_EXTENSION = "clazy_v%s-msvc%s" % (CLAZY_VERSION, MSVC_VERSION)
CLAZY_ZIP = CLAZY_ZIP_WITHOUT_EXTENSION + '.zip'
CLAZY_SRC_ZIP = "clazy_v%s-src.zip" % CLAZY_VERSION

IS_FIRST_RUN = (MSVC_VERSION == '2015')


def run_command(cmd, abort_on_error = True):
    print cmd
    success = (os.system(cmd) == 0)
    if abort_on_error and not success:
        sys.exit(1)

    return success

def copy(src, dest):
    run_command('cp %s %s' % (src, dest))

def check_env():
    if MSVC_VERSION not in ['2015', '2017']:
        print "Error: Set MSVC_VERSION to a proper value. Exiting..."
        sys.exit(1)

    if not LLVM_INSTALL_DIR:
        print "Error: Set LLVM_INSTALL_DIR to a proper value. Exiting..."
        sys.exit(1)

    if IS_FIRST_RUN:
        run_command('rm -rf ' + PACKAGE_DIR)

def clone_clazy(sha1, work_dir):
    if os.path.exists(work_dir):
        run_command("rm -rf " + work_dir)
    run_command("git clone %s %s" % (CLAZY_REPO_URL, work_dir))
    run_command("git checkout " + sha1)

def build_clazy():
    cmd = 'cmake -DCMAKE_INSTALL_PREFIX=%s -DCMAKE_BUILD_TYPE=Release -DCLANG_LIBRARY_IMPORT=%s\lib\clang.lib -G "NMake Makefiles JOM" .' % (LLVM_INSTALL_DIR, LLVM_INSTALL_DIR)
    run_command(cmd)
    run_command('jom')
    run_command('jom install')

def copy_files(work_dir):

    if not os.path.exists(PACKAGE_DIR_WIN):
        os.mkdir(PACKAGE_DIR_WIN)

    os.mkdir(PACKAGE_DIR_WIN + 'clazy')
    os.mkdir(PACKAGE_DIR_WIN + 'clazy/share')
    os.mkdir(PACKAGE_DIR_WIN + 'clazy/bin')
    os.mkdir(PACKAGE_DIR_WIN + 'clazy/bin/clang')

    copy("../windows-package/clazy-cl.bat", PACKAGE_DIR + 'clazy/bin')
    copy("../windows-package/clazy.bat", PACKAGE_DIR + 'clazy/bin')
    copy("../windows-package/LICENSE-CLAZY.txt", PACKAGE_DIR + 'clazy')
    copy("../windows-package/LICENSE-LLVM.TXT", PACKAGE_DIR + 'clazy')
    copy("../windows-package/README.txt", PACKAGE_DIR + 'clazy')
    copy("../README.md", PACKAGE_DIR + 'clazy/README-CLAZY.md')
    copy(LLVM_INSTALL_DIR + '/bin/clang.exe', PACKAGE_DIR + 'clazy/bin/clang/')
    copy(LLVM_INSTALL_DIR + '/bin/clang.exe', PACKAGE_DIR + 'clazy/bin/clang/clang-cl.exe')
    copy(LLVM_INSTALL_DIR + '/bin/ClangLazy.dll', PACKAGE_DIR + 'clazy/bin/clang/')
    copy(LLVM_INSTALL_DIR + '/bin/clazy-standalone.exe', PACKAGE_DIR + 'clazy/bin/clang/')

    run_command("cp -r %s/lib/clang/ %s" % (LLVM_INSTALL_DIR, PACKAGE_DIR + 'clazy/bin/lib/'))
    run_command("cp -r %s/share/doc/ %s" %  (LLVM_INSTALL_DIR, PACKAGE_DIR + 'clazy/share/'))
    run_command('unix2dos %s' % PACKAGE_DIR + 'clazy/bin/clazy-cl.bat')
    run_command('unix2dos %s' % PACKAGE_DIR + 'clazy/bin/clazy.bat')

def zip_package():
    os.chdir(PACKAGE_DIR_WIN)
    run_command('zip -r %s clazy/' % (CLAZY_ZIP))
    run_command('rm -rf clazy')

    if IS_FIRST_RUN:
        run_command('wget --no-check-certificate https://github.com/KDE/clazy/archive/%s.zip -O %s' % (CANDIDATE_SHA1, CLAZY_SRC_ZIP))
        run_command('sha1sum %s > sums.txt' % CLAZY_ZIP)
        run_command('sha256sum %s >> sums.txt' % CLAZY_ZIP)
        run_command('sha1sum %s >> sums.txt' % CLAZY_SRC_ZIP)
        run_command('sha256sum %s >> sums.txt' % CLAZY_SRC_ZIP)
    else:
        run_command('sha1sum %s >> sums.txt' % CLAZY_ZIP)
        run_command('sha256sum %s >> sums.txt' % CLAZY_ZIP)

    run_command("unzip %s -d %s" % (CLAZY_ZIP, CLAZY_ZIP_WITHOUT_EXTENSION))
    print "Don't forget to delete %s after testing" % CLAZY_ZIP_WITHOUT_EXTENSION
    os.chdir('..')

check_env()
clone_clazy(CANDIDATE_SHA1, CLAZY_WORK_DIR)
os.chdir(CLAZY_WORK_DIR)
build_clazy()
copy_files(CLAZY_WORK_DIR)
zip_package()
