import subprocess
import re
import os

from .os_utils import get_command_output
from .run_platform import isMacOS, isWindows

c_headerpath = None
try:
    result = subprocess.run(['clang', '-v'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, text=True)
    match = re.search(r'Selected .* installation: (.*)', result.stderr)
    if match:
        c_headerpath = match.group(1).strip()
except:
    pass


class QtInstallation:
    def __init__(self):
        self.int_version = 000
        self.qmake_header_path = "/usr/include/qt/"
        self.qmake_lib_path = "/usr/lib"

    def compiler_flags(self, cxx_args: str, qt_namespaced: bool, module_includes = False):
        extra_includes = ''
        if isMacOS():
            extra_includes = " -I%s/QtCore.framework/Headers" % self.qmake_lib_path
            extra_includes += " -iframework %s" % self.qmake_lib_path

        # Also include the modules folders
        qt_modules_includes = []
        if module_includes:
            qt_modules_includes = ["-isystem " + self.qmake_header_path + "/" + f for f in next(os.walk(self.qmake_header_path))[1]]
        additional_args = ""
        if c_headerpath:
            additional_args = "-isystem " + c_headerpath + "/include "
        if cxx_args:
            additional_args += cxx_args + " "
        if qt_namespaced:
            additional_args += " -DQT_NAMESPACE=MyQt "

        return additional_args + "-isystem " + self.qmake_header_path + ("" if isWindows() else " -fPIC") + " -L " + self.qmake_lib_path + ' ' + extra_includes + ' '.join(qt_modules_includes)

def find_qt_installation(major_version, qmakes, verbose: bool) -> QtInstallation:
    installation = QtInstallation()

    for qmake in qmakes:
        qmake_version_str, success = get_command_output(qmake + " -query QT_VERSION", verbose)
        if success and qmake_version_str.startswith(str(major_version) + "."):
            qmake_header_path = get_command_output(qmake + " -query QT_INSTALL_HEADERS", verbose)[0].strip()
            qmake_lib_path = get_command_output(qmake + " -query QT_INSTALL_LIBS", verbose)[0].strip()
            if qmake_header_path:
                installation.qmake_header_path = qmake_header_path
                if qmake_lib_path:
                    installation.qmake_lib_path = qmake_lib_path
                ver = qmake_version_str.split('.')
                installation.int_version = int(
                    ver[0]) * 10000 + int(ver[1]) * 100 + int(ver[2])
                if verbose:
                    print("Found Qt " + str(installation.int_version) + " using qmake " + qmake)
            break

    if installation.int_version == 0:
        print("Error: Couldn't find a Qt" + str(major_version) + " installation")
    return installation

_qt5_installation = None
_qt6_installation = None

def qt_installation(major_version, verbose: bool):
    global _qt6_installation
    global _qt5_installation
    if major_version == 6:
        if _qt6_installation is None:
            _qt6_installation = find_qt_installation(6, ["QT_SELECT=6 qmake", "qmake-qt6", "qmake", "qmake6"], verbose)
        return _qt6_installation
    elif major_version == 5:
        if _qt5_installation is None:
            _qt5_installation = find_qt_installation(5, ["QT_SELECT=5 qmake", "qmake-qt5", "qmake", "qmake5"], verbose)
        return _qt5_installation
    return None
