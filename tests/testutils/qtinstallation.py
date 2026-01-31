import subprocess
import re
import os

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
