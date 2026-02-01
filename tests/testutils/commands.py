import os

from . import Args
from .run_platform import isWindows, isMacOS
from .test import Test
from .qtinstallation import QtInstallation, qt_installation

_export_fixes_argument = "-Xclang -plugin-arg-clazy -Xclang export-fixes"
_suppress_line_numbers_opt = " -fno-diagnostics-show-line-numbers"

def library_name():
    if 'CLAZYPLUGIN_CXX' in os.environ: # Running tests uninstalled
        return os.environ['CLAZYPLUGIN_CXX']
    elif isWindows():
        return 'ClazyPlugin.dll'
    elif isMacOS():
        return 'ClazyPlugin.dylib'
    else:
        return 'ClazyPlugin.so'


def clang_tidy_plugin_name():
    if 'CLANGTIDYPLUGIN_CXX' in os.environ: # Running tests uninstalled
        return os.environ['CLANGTIDYPLUGIN_CXX']
    return "ClazyClangTidy.so"


def clazy_cpp_args(cppStandard):
    return ' -Wno-unused-value -Qunused-arguments -std=' + cppStandard + ' '


def more_clazy_standalone_args():
    if 'CLANG_BUILTIN_INCLUDE_DIR' in os.environ:
        return ' -I ' + os.environ['CLANG_BUILTIN_INCLUDE_DIR']
    return ''




def clang_name():
    return os.getenv('CLANGXX', 'clang')


def clazy_command(test: Test, cpp_standard: str, qt: QtInstallation, filename: str, config: Args):
    if test.isScript():
        return filename  # is absolute path

    if 'CLAZY_CXX' in os.environ:  # In case we want to use clazy.bat
        result = os.environ['CLAZY_CXX']
    else:
        result = clang_name() + " -Xclang -load -Xclang " + library_name() + " -Xclang -add-plugin -Xclang clazy "
    result += clazy_cpp_args(cpp_standard) + qt.compiler_flags(config.cxx_args, config.qt_namespaced, test.qt_modules_includes) + _suppress_line_numbers_opt

    if test.only_qt:
        result = result + " -Xclang -plugin-arg-clazy -Xclang only-qt "
    if test.qt_developer:
        result = result + " -Xclang -plugin-arg-clazy -Xclang qt-developer "
    if test.extra_definitions:
        result += " " + test.extra_definitions

    result = result + " -c "

    result = result + test.flags + \
        " -Xclang -plugin-arg-clazy -Xclang " + ','.join(test.checks) + " "
    if test.has_fixits:
        result += _export_fixes_argument + " "
    result += filename

    return result


def clang_tidy_command(test: Test, cpp_standard, qt, filename, config: Args):
    command = f"clang-tidy {filename}"
    # disable all checks, re-enable clazy ones
    checks = ','.join("clazy-" + check for check in test.checks)
    command += f" -checks='-*,{checks}' -header-filter='.*' -system-headers -load='{clang_tidy_plugin_name()}'"

    # Add extra compiler flags
    command += " -- "
    command += f" {test.flags} "
    command += clazy_cpp_args(cpp_standard)
    command += qt.compiler_flags(config.cxx_args, config.qt_namespaced, test.qt_modules_includes)
    command += _suppress_line_numbers_opt
    if test.extra_definitions:
        command += " " + test.extra_definitions
    return command


def dump_ast_command(test: Test, cpp_standard, qt_major_version):
    return "clang -std=" + cpp_standard + " -fsyntax-only -Xclang -ast-dump -fno-color-diagnostics -c " + qt_installation(qt_major_version).compiler_flags(test.qt_modules_includes) + " " + test.flags + " " + test.filename


def clazy_standalone_command(test: Test, cpp_standard: str, qt: QtInstallation, config: Args):
    result = " -- " + clazy_cpp_args(cpp_standard) + \
             qt.compiler_flags(config.cxx_args, config.qt_namespaced, test.qt_modules_includes) + " " + test.flags + more_clazy_standalone_args()
    result = " -checks=" + ','.join(test.checks) + " " + result + _suppress_line_numbers_opt

    if test.has_fixits:
        result = " -export-fixes=" + \
            test.yamlFilename(is_standalone=True) + result

    if test.only_qt:
        result = " -only-qt " + result

    if test.qt_developer:
        result = " -qt-developer " + result

    if test.header_filter:
        result = " -header-filter " + test.header_filter + " " + result

    if test.ignore_dirs:
        result = " -ignore-dirs " + test.ignore_dirs + " " + result

    if test.extra_definitions:
        result += " " + test.extra_definitions

    return result
