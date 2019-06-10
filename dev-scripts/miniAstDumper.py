#!/usr/bin/env python

import cbor, sys, time


class FunctionCall:
    def __init__(self):
        self.callee_name = ""

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

    if 'stuff' in cborData:
        for stuff in cborData['stuff']:
            if 'type' in stuff:
                if stuff['type'] == 31: # CXXRecordDecl
                    cxxclass = CXXClass()
                    cxxclass.id = stuff['id']
                    cxxclass.qualified_name = stuff['name']

                    if 'methods' in stuff:
                        for m in stuff['methods']:
                            method = CXXMethod()
                            methods.id = m['id']
                            methods.qualified_name = m['name']
                            cxxclass.methods.append(method)



                    globalAST.cxx_classes.append(cxxclass)
                elif stuff['type'] == 48: # CallExpr
                    funccall = FunctionCall()
                    funccall.callee_name = stuff['calleeName']
                    globalAST.function_calls.append(funccall)




load_cbor(sys.argv[1], _globalAST)


for c in _globalAST.cxx_classes:
    print(c.qualified_name)

#print(cborData)
