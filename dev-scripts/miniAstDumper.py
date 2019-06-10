#!/usr/bin/env python

import cbor, sys, time, os

class SourceLocation:
    def __init__(self):
        self.filename = ""
        self.lineNumber = -1
        self.columnNumber = -1

    def asString(self):
        return self.filename + ":" + str(self.lineNumber) + ":" + str(self.columnNumber)

    def dump(self):
        print(self.asString())


class FunctionCall:
    def __init__(self):
        self.callee_name = ""
        self.loc_start = SourceLocation()

class CXXMethod:
    def __init__(self):
        self.id = 0
        self.qualified_name = ""
        self.method_flags = 0

class CXXClass:
    def __init__(self):
        self.id = 0
        self.qualified_name = ""
        self.methods = []
        self.class_flags = 0

class GlobalAST:
    def __init__(self):
        self.cxx_classes = []
        self.function_calls = []


_globalAST = GlobalAST()

def parse_loc(cborLoc, file_map, tu_cwd):
    loc = SourceLocation()
    loc.filename = file_map[str(cborLoc['fileId'])]

    # Make absolute
    if not loc.filename.startswith('/'): # TODO windows
        loc.filename = tu_cwd + '/' + loc.filename

    # Normalize
    loc.filename = os.path.normpath(loc.filename)

    loc.lineNumber = cborLoc['line']
    loc.columnNumber = cborLoc['column']
    return loc

def read_file(filename):
    f = open(filename, 'rb')
    contents = f.read()
    f.close()
    return contents

def read_cbor(filename):
    contents = read_file(filename);
    return cbor.loads(contents)

def load_cbor(filename, globalAST):
    cborData = read_cbor(filename)

    file_map = {}

    current_tu_cwd = cborData['cwd']

    # populate the file map
    if 'files' in cborData:
        for fileId in cborData['files'].keys():
            file_map[fileId] = cborData['files'][fileId]


    if 'stuff' in cborData:
        for stuff in cborData['stuff']:
            if 'type' in stuff:
                if stuff['type'] == 31: # CXXRecordDecl
                    cxxclass = CXXClass()
                    # cxxclass.id = stuff['id']
                    cxxclass.qualified_name = stuff['name']

                    if 'methods' in stuff:
                        for m in stuff['methods']:
                            method = CXXMethod()
                            method.id = m['id']
                            method.qualified_name = m['name']
                            cxxclass.methods.append(method)

                    globalAST.cxx_classes.append(cxxclass)
                elif stuff['type'] == 48: # CallExpr
                    funccall = FunctionCall()
                    funccall.callee_name = stuff['calleeName']
                    funccall.loc_start = parse_loc(stuff['loc'], file_map, current_tu_cwd)
                    globalAST.function_calls.append(funccall)


def get_class_by_name(qualified_name):
    result = []
    for c in _globalAST.cxx_classes:
       if c.qualified_name == qualified_name:
           result.append(c)
    return result

def get_calls_by_name(callee_name):
    result = []
    for f in _globalAST.function_calls:
       if f.callee_name == callee_name:
           result.append(f)
    return result

load_cbor(sys.argv[1], _globalAST)

#string_class = get_class_by_name("QString")[0]

for f in get_calls_by_name("QObject::connect"):
    print(f.loc_start.dump())

#for f in _globalAST.function_calls:
 #   print(f.callee_name)

#for m in string_class.methods:
 #   print(m.qualified_name)

#for c in _globalAST.cxx_classes:
 #   print(c.qualified_name)

#print(cborData)
