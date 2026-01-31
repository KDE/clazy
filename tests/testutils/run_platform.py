from sys import platform as _platform

def isWindows():
    return _platform == 'win32'

def isMacOS():
    return _platform == 'darwin'

def isLinux():
    return _platform.startswith('linux')

