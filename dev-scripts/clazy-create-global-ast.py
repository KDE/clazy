#!/usr/bin/env python

_license_text = \
"""/*
  This file is part of the clazy static checker.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
"""

import cbor, sys, time, os, pickle

ClassFlag_None = 0
ClassFlag_QObject = 1

MethodFlag_None = 0
MethodFlag_Signal = 1

StuffType_FunctionDecl = 1 # excluding methods
StuffType_MethodDecl = 2
StuffType_RecordDecl = 4
StuffType_CallExpr = 8

_dump_stats = False
_check_sanity = False

def printWarning(warning):
    print('WARNING: ' + warning)

class TranslationUnit:
    def __init__(self, cborData, globalAST):
        self.__file_map = {} # The file-map in TU terms
        self.__cwd = cborData['cwd']
        self.__function_map = {} # Function map in tu terms
        self.__class_map = {}
        self.globalAST = globalAST

        # populate the file map
        if 'files' in cborData:
            for fileId in cborData['files'].keys():
                self.__file_map[fileId] = absolute_file(cborData['files'][fileId], self.__cwd)

    def function(self, local_id):
        return self.__function_map[local_id]

    def cxx_class(self, local_id):
        return self.__class_map[local_id]

    def add_function(self, func, local_id):
        self.__function_map[local_id] = func

    def add_class(self, cxxclass, local_id):
        self.__class_map[local_id] = cxxclass

    def contains_function(self, local_id):
        return local_id in self.__function_map

    def filename_for_id(self, fileId):
        return self.__file_map[str(fileId)]


class SourceLocation:
    def __init__(self, globalAST):
        self.file_global_id = ""
        self.lineNumber = -1
        self.columnNumber = -1
        self.spelling_file_global_id = ""
        self.spellingLineNumber = -1
        self.spellingColumnNumber = -1
        self.globalAST = globalAST

    def asString(self):
        result = "%s:%d:%d" % (self.filename(), self.lineNumber, self.columnNumber)

        if self.isMacro(): # if the loc is the same then the spelling filename is the same in both, so don't add it. They only diverge on line/column, as a macro can't be defined in two files
            result += ":%d:%d" % (self.spellingLineNumber, self.spellingColumnNumber)
        return result

    def filename(self):
        return self.globalAST.get_filename(self.file_global_id)

    def isMacro(self):
        return self.spelling_file_global_id != ""

    def spelling_filename(self):
        return self.globalAST.get_filename(self.spelling_file_global_id)

    def hash(self):
        return "%s%d%d%d%d" % (self.file_global_id, self.lineNumber, self.columnNumber, self.spellingLineNumber, self.spellingColumnNumber)

    def dump(self):
        print(self.asString())

    def check_sanity(self):
        if self.lineNumber < 0 or self.columnNumber < 0:
            print("SourceLocation::check_sanity: loc numbers are invalid! " + self.lineNumber + " ; " + self.columnNumber)

        if not self.filename:
            print("SourceLocation::check_sanity: loc filename is invalid!")

    def from_local_cbor(self, cborData, localCborLoader):
        tu = localCborLoader.current_tu

        filename = tu.filename_for_id(cborData['fileId'])

        new_file_global_id = globalAST.global_id_for_filename(filename)
        if not new_file_global_id:
            new_file_global_id = localCborLoader.get_next_file_global_id()

        self.file_global_id = new_file_global_id
        self.globalAST.add_filename(filename, self.file_global_id)
        self.lineNumber = cborData['line']
        self.columnNumber = cborData['column']
        if 'spellingFileId' in cborData:

            spelling_filename = tu.filename_for_id(cborData['spellingFileId'])
            new_spelling_file_global_id = globalAST.global_id_for_filename(spelling_filename)
            if not new_spelling_file_global_id:
                new_spelling_file_global_id = localCborLoader.get_next_file_global_id()

            self.spelling_file_global_id = self.globalAST.add_filename(spelling_filename, new_spelling_file_global_id)
            self.spellingLineNumber = cborData['spellingLine']
            self.spellingColumnNumber = cborData['spellingColumn']

    def from_global_cbor(self, cborData):
        self.file_global_id = cborData['fileId']
        self.lineNumber = cborData['line']
        self.columnNumber = cborData['column']
        if 'spellingFileId' in cborData:
            self.spelling_file_global_id = cborData['spellingFileId']
            self.spellingLineNumber = cborData['spellingLine']
            self.spellingColumnNumber = cborData['spellingColumn']

    def to_global_cbor(self):
        cborData = {}
        cborData['line'] = self.lineNumber
        cborData['column'] = self.columnNumber
        cborData['fileId'] = self.file_global_id

        if self.spellingLineNumber:
            cborData['spellingLine'] = self.spellingLineNumber
            cborData['spellingColumn'] = self.spellingColumnNumber
            cborData['spellingFileId'] = self.spelling_file_global_id

        return cborData

class FunctionCall:
    def __init__(self, globalAST):
        self.callee_global_id = ""
        self.loc_start = None
        self.globalAST = globalAST

    def check_sanity(self):
        if not self.callee_global_id:
            print("FunctionCall::check_sanity: callee_global_id is empty, loc= %s" % (self.loc_start.asString()))
        self.loc_start.check_sanity()

    def hash(self):
        return self.callee_global_id + self.loc_start.hash()

    def from_local_cbor(self, cborData, localCborLoader):
        tu = localCborLoader.current_tu
        self.loc_start = SourceLocation(self.globalAST)
        self.loc_start.from_local_cbor(cborData['loc'], localCborLoader)

        callee_local_id = cborData['calleeId']
        if tu.contains_function(callee_local_id):
            self.callee_global_id = tu.function(callee_local_id).global_id
            if not self.callee_global_id:
                print("FunctionCall::check_sanity: callee_global_id is empty for callee_local_id= %d, loc= %s" % (callee_local_id, self.loc_start.asString()))
        else:
            printWarning("Could not find function with local tu id= %d, loc= %s" % (callee_local_id, self.loc_start.asString()))

    def from_global_cbor(self, cborData):
        self.loc_start = SourceLocation(self.globalAST)
        self.loc_start.from_global_cbor(cborData['loc'])
        self.callee_global_id = cborData['callee_global_id']

    def to_global_cbor(self):
        cborData = {}
        cborData['callee_global_id'] = self.callee_global_id
        cborData['loc'] = self.loc_start.to_global_cbor()
        return cborData

class Function:
    def __init__(self, globalAST):
        self.global_id = ""
        self.qualified_name = ""
        self.loc_start = None
        self.specialization_loc = None
        self.globalAST = globalAST
        self.template_args = ""
        self.__isMethod = False

    def isMethod(self):
        return self.__isMethod

    def check_sanity(self):
        if not self.global_id:
            print("Function::check_sanity: global_id is empty for %s, hash in ast?=%s" % (self.qualified_name, str(hash_in_ast)))

        if not hash_in_ast:
            print('Function::check_sanity: not known by globalAST. %s' % (self.qualified_name))

        if not self.qualified_name:
            print("Function::check_sanity: qualified_name is empty!")

    def asString(self):
        return self.qualified_name + " : " + self.loc_start.asString()

    def hash(self):
        result =  self.qualified_name + self.loc_start.hash() + self.template_args
        if self.specialization_loc:
            result += " : " + self.specialization_loc.hash()
        return result

    def from_local_cbor(self, cborData, localCborLoader, assign_id):
        self.qualified_name = cborData['name']
        self.loc_start = SourceLocation(self.globalAST)
        #self.loc_start.from_local_cbor(cborData['loc'], localCborLoader)

        if 'template_args' in cborData:
            self.template_args = cborData['template_args']

        # In case it's a template specialization we need to add the specialization location to the hash, otherwise different specializations will have the loc of the template and have the same hash
        if 'specialization_loc' in cborData:
            self.specialization_loc = SourceLocation(self.globalAST)
            self.specialization_loc.from_local_cbor(cborData['specialization_loc'], localCborLoader)

        if assign_id:
            self.assign_global_id(self.hash(), localCborLoader, cborData['id'])

        localCborLoader.current_tu.add_function(self, cborData['id'])

    def assign_global_id(self, hash, localCborLoader, local_id):
        # Function might already been added by another TU, in that case don't generate another global id
        if hash in self.globalAST.function_by_hash:
            self.global_id = globalAST.function_by_hash[hash].global_id
        else:
            self.global_id = localCborLoader.get_next_function_global_id() # Attribute a sequential id, that's unique across TUs

    def from_global_cbor(self, cborData):
        self.global_id = cborData['global_id']
        self.qualified_name = cborData['qualified_name']
        self.loc_start = SourceLocation(self.globalAST)
        self.loc_start.from_global_cbor(cborData['loc_start'])
        self.__isMethod =  cborData['is_method']

        if 'template_args' in cborData:
            self.template_args = cborData['template_args']

        # In case it's a template specialization we need to add the specialization location to the hash, otherwise different specializations will have the loc of the template and have the same hash
        if 'specialization_loc' in cborData:
            self.specialization_loc = SourceLocation(self.globalAST)
            self.specialization_loc.from_global_cbor(cborData['specialization_loc'])

    def to_global_cbor(self):
        cborData = {}
        cborData['is_method'] = self.__isMethod
        if self.template_args:
            cborData['template_args'] = self.template_args

        if self.specialization_loc:
            cborData['specialization_loc'] = self.specialization_loc.to_global_cbor()

        cborData['loc_start'] = self.loc_start.to_global_cbor()
        cborData['global_id'] = self.global_id
        cborData['qualified_name'] = self.qualified_name
        return cborData

class CXXMethod(Function):
    def __init__(self, globalAST, cxx_class):
        Function.__init__(self, globalAST)
        self.__isMethod = True
        self.method_flags = 0
        self.overrides = [] # id of methods this overrides
        self.overridden_by = [] # id of methods that override this
        self.cxx_class = cxx_class
        if not cxx_class:
            printWarning("Method is orphan!!")

    def hash(self):
        return Function.hash(self) + str(self.method_flags)

    def from_local_cbor(self, cborData, localCborLoader):
        Function.from_local_cbor(self, cborData, localCborLoader, False)
        if 'method_flags' in cborData:
            self.method_flags = cborData['method_flags']
        self.assign_global_id(self.hash(), localCborLoader, cborData['id'])

        for ov in cborData['overrides']:
            # This results in ignoring overridded functions in system headers. Problem?
            if localCborLoader.current_tu.contains_function(ov):
                localCborLoader.current_tu.function(ov).overridden_by.append(self.global_id)
                self.overrides.append(localCborLoader.current_tu.function(ov).global_id)

    def from_global_cbor(self, cborData):
        Function.from_global_cbor(self, cborData)
        if 'method_flags' in cborData:
            self.method_flags = cborData['method_flags']



    def to_global_cbor(self):
        cborData = Function.to_global_cbor(self)
        if self.method_flags:
            cborData['method_flags'] = self.method_flags
        return cborData


class CXXClass:
    def __init__(self, globalAST):
        self.global_id = ""
        self.globalAST = globalAST
        self.qualified_name = ''
        self.loc_start = None
        self.methods = []
        self.class_flags = 0
        self.template_args = ""

    def check_sanity(self):
        #if self.id == -1:
        #    print("CXXClass::check_sanity: id is empty!")

        if not self.qualified_name:
            print("CXXClass::check_sanity: qualified_name is empty!")

        for m in self.methods:
            m.check_sanity()

    def hash(self):
        return self.qualified_name + self.loc_start.hash() + self.template_args

    def from_local_cbor(self, cborData, localCborLoader):
        self.global_id = localCborLoader.get_next_class_id()
        self.qualified_name = cborData['name']
        self.loc_start = SourceLocation(self.globalAST)
        self.loc_start.from_local_cbor(cborData['loc'], localCborLoader)

        if 'template_args' in cborData:
            self.template_args = cborData['template_args']

        if 'class_flags' in cborData:
            self.class_flags = cborData['class_flags']

        if 'methods' in cborData:
            for m in cborData['methods']:
                method = CXXMethod(self.globalAST, self)
                method.from_local_cbor(m, localCborLoader)
                self.methods.append(method)

        localCborLoader.current_tu.add_class(self, 1) #FIXME

    def from_global_cbor(self, cborData):
        self.global_id = cborData['global_id']
        self.qualified_name = cborData['qualified_name']
        self.loc_start = SourceLocation(self.globalAST)
        self.loc_start.from_global_cbor(cborData['loc_start'])

        if 'template_args' in cborData:
            self.template_args = cborData['template_args']

        if 'class_flags' in cborData:
            self.class_flags = cborData['class_flags']

        if 'methods' in cborData:
            for m in cborData['methods']:
                method = CXXMethod(globalAST, self)
                method.from_global_cbor(m)
                self.methods.append(method)

    def to_global_cbor(self):
        cborData = {}
        cborData['methods'] = []
        if self.class_flags:
            cborData['class_flags'] = self.class_flags
        if self.template_args:
            cborData['template_args'] = self.template_args

        cborData['global_id'] = self.global_id
        cborData['loc_start'] = self.loc_start.to_global_cbor()
        cborData['qualified_name'] = self.qualified_name

        for m in self.methods:
            cborData['methods'].append(m.to_global_cbor())
        return cborData

class GlobalAST:
    def __init__(self):
        self.cxx_classes = [] # indexed by hash
        self.function_calls = []
        self.function_map = {} # Indexed by function id, value is a Function object
        self.__filename_map = {} # indexed by a sequential ID
        self.function_by_hash = {} # Function by hash

    def get_filename(self, global_file_id):
        return self.__filename_map[global_file_id]

    def contains_filename(self, filename):
        return filename in self.__filename_map.values()

    def global_id_for_filename(self, filename):
        for global_id in self.__filename_map:
            if self.__filename_map[global_id] == filename: # O(n)
                return global_id
        return ""

    def add_filename(self, filename, global_id):
        self.__filename_map[str(global_id)] = filename # Str so it can be stored in cbor as key

    def add_function_call(self, funcall):
        # If avoid_duplicates is False, we save memory. We're processing a curated global AST.
        self.function_calls.append(funcall)

    def add_function(self, func):
        if not func.global_id:
            print("IMPOSSIBLE!")

        self.function_by_hash[func.hash()] = func
        self.function_map[func.global_id] = func
        return True

    def add_class(self, cxxclass):
        self.cxx_classes.append(cxxclass)

    def dump_stats(self):
        print("Stats:")
        print("num classes=%d, num calls=%d, num_functions=%d, num_files=%d" % (len(self.cxx_classes), len(self.function_calls), len(self.function_map), len(self.__filename_map)))

    def check_sanity(self):
        print("GlobalAST: Checking sanity...")
        self.dump_stats()
        for c in self.cxx_classes:
            c.check_sanity()
        for fc in self.function_calls:
            fc.check_sanity()
        for f in self.function_map:
           self.function_map[f].check_sanity()

    def get_functions(self):
        return self.function_map.values()

    def from_global_cbor(self, cborData):
        if 'files' in cborData:
            self.__filename_map = cborData['files']

        if 'classes' in cborData:
            for c in cborData['classes']:
                cxx_class = CXXClass(globalAST)
                cxx_class.from_global_cbor(c)
                self.add_class(cxx_class)
                for method in cxx_class.methods:
                    self.add_function(method)
        if 'functions' in cborData:
            for f in cborData['functions']:
                if not f['is_method']:
                    func = Function(globalAST)
                    func.from_global_cbor(f)
                    self.add_function(func)

        if 'function_calls' in cborData:
            for fc in cborData['function_calls']:
                funccall = FunctionCall(globalAST)
                funccall.from_global_cbor(fc)
                self.add_function_call(funccall)

    def to_global_cbor(self):
        cborData = {}
        cborData['classes'] = []
        cborData['function_calls'] = []
        cborData['functions'] = []
        cborData['files'] = self.__filename_map
        for cxx_class in self.cxx_classes:
            cborData['classes'].append(cxx_class.to_global_cbor())

        for func_call in self.function_calls:
            cborData['function_calls'].append(func_call.to_global_cbor())

        for func_global_id in self.function_map:
            func = self.function_map[func_global_id]
            if not func.isMethod(): # Methods already dumped by the class
                cborData['functions'].append(func.to_global_cbor())

        return cborData

    def dump_global_cbor_to_file(self, filename):
        cborData = self.to_global_cbor()
        f = open(filename, 'wb')
        cbor.dump(cborData, f)
        f.close()

def read_file(filename):
    f = open(filename, 'rb')
    contents = f.read()
    f.close()
    return contents

def read_cbor(filename):
    contents = read_file(filename)
    return cbor.loads(contents)

class LocalCborLoader:
    def __init__(self, globalAST, checkSanity):
        self.next_file_id = 1
        self.next_function_id = 1
        self.next_class_id = 1
        self.globalAST = globalAST
        self.checkSanity = checkSanity
        self.current_tu = None
        self.__function_by_hash = {} # Set, by hash
        self.__function_call_map = {} # indexed by hash
        self.cxx_classes = {} # indexed by hash

    def get_next_file_global_id(self):
        result = self.next_file_id
        self.next_file_id += 1
        return str(result) # str so it can be used as a key in cbor

    def load_local_cbor_from_files(self, filenames):
        i = 0
        count = len(filenames)
        for filename in filenames:
            i += 1
            print("Processing %s (%d of %d)" % (filename, i, count))
            self.load_local_cbor_from_file(filename)
        if self.checkSanity:
            self.globalAST.check_sanity()

    def load_local_cbor_from_file(self, filename):
        cborData = read_cbor(filename)
        self.load_local_cbor(cborData)

    def add_class(self, cxxclass):
        h = cxxclass.hash()
        if h in self.cxx_classes:
            return False
        else:
            self.cxx_classes[h] = cxxclass
            self.globalAST.add_class(cxxclass)
            return True

    def add_function(self, func):
        h = func.hash()
        if h in self.__function_by_hash:
            return False
        else:
            self.__function_by_hash[h] = True
            self.globalAST.add_function(func)
            return True

    def load_local_cbor(self, cborData):
        self.current_tu = TranslationUnit(cborData, self.globalAST)
        if 'stuff' in cborData:
            # Process classes and methods
            for stuff in cborData['stuff']:
                if 'type' in stuff:
                    if stuff['type'] == 33:#StuffType_RecordDecl:
                        cxxclass = CXXClass(self.globalAST)
                        cxxclass.from_local_cbor(stuff, self)
                        if self.add_class(cxxclass):
                            for method in cxxclass.methods:
                                self.add_function(method)

                    elif stuff['type'] == 50:#StuffType_FunctionDecl:
                        func = Function(self.globalAST)
                        func.from_local_cbor(stuff, self, True)
                        self.add_function(func)

            # Process CallExprs
            #for stuff in cborData['stuff']:
                if 'stmt_type' in stuff and (stuff['stmt_type'] == 122 or stuff['stmt_type'] == 124): # function call #StuffType_CallExpr: # CallExpr
                    funccall = FunctionCall(self.globalAST)
                    funccall.from_local_cbor(stuff, self)

                    # We're processing non-curated tu ASTs, so avoid duplicates:
                    h = funccall.hash()
                    if h not in self.__function_call_map:
                        self.__function_call_map[h] = True
                        self.globalAST.add_function_call(funccall)
        if _dump_stats:
            self.globalAST.dump_stats()
            self.dump_stats()

    def dump_stats(self):
        print("LocalCborLoader stats:")
        print("classes=%d; calls=%d; functions=%d" % (len(self.cxx_classes), len(self.__function_call_map), len(self.__function_by_hash)))

    def get_next_function_global_id(self):
        result = self.next_function_id
        self.next_function_id += 1
        return str(result) # str, so it can be a key in cbor

    def get_next_class_id(self):
        result = self.next_class_id
        self.next_class_id += 1
        return str(result) # str, so it can be a key in cbor


def absolute_file(filename, tu_cwd):
    if not filename.startswith('/'): # TODO windows
        filename = tu_cwd + '/' + filename

    # Normalize
    filename = os.path.normpath(filename)
    return filename

def get_all_method_names(ast):
    total_methods = []
    for c in ast.cxx_classes.values():
        for m in c.methods:
            total_methods.append(m.qualified_name)
    return total_methods

def print_qobjects(ast):
    for c in ast.cxx_classes.values():
        if c.class_flags & ClassFlag_QObject:
            print(c.qualified_name)

# Returns a list of (function, call count)
def calculate_call_counts(ast):
    print("Calculating call counts...")

    calls = {} # callee_global_id => # calls

    for fun in ast.function_map:
        calls[fun] = 0

    for c in ast.function_calls:
        if c.callee_global_id in ast.function_map:
            calls[c.callee_global_id] += 1

            fun = ast.function_map[c.callee_global_id]

            # Consider a method called when super method is called
            if isinstance(fun, CXXMethod):
                for ov in fun.overridden_by:
                    calls[ov] += 1

        else:
            print("Call not found %s" % (c.callee_global_id))

    return list(map(lambda x: (ast.function_map[x], calls[x]), calls))

def print_unused_signals(ast):
    for c in ast.cxx_classes.values():
        for m in c.methods:
            if (m.method_flags & MethodFlag_Signal) and m.num_called == 0:
                print("Signal " + m.qualified_name + " never called")

def print_signal_counts(ast):
    for c in ast.cxx_classes:
        for m in c.methods:
            if m.method_flags & MethodFlag_Signal:
                print("Signal %s: %d %s" % (m.qualified_name, m.num_called, ("UNUSED" if  m.num_called == 0 else "")))


def print_calls(ast):
    for c in ast.function_calls:
        if c.callee_global_id in ast.function_map:
            func = ast.function_map[c.callee_global_id]
            print('CALL: ' + func.qualified_name)
        else:
            print("Call not found %s" % (c.callee_global_id))


#def get_class_by_name(qualified_name):
#    result = []
#    for c in _globalAST.cxx_classes:
#       if c.qualified_name == qualified_name:
#           result.append(c)
#    return result

#def get_calls_by_name(callee_name):
#    result = []
#    for f in _globalAST.function_calls:
#if f.callee_name == callee_name:
#           result.append(f)
    #return result




# load_local_cbor(sys.argv[1], _globalAST)


#print ("Functions: " + str(len(_globalAST.functions)))



#print(str(total_methods))


#_globalAST.check_sanity()

#string_class = get_class_by_name("QString")[0]

#for f in get_calls_by_name("QObject::connect"):
    #print(f.loc_start.dump())

#for f in _globalAST.function_calls:
 #   print(f.callee_name)

#for m in string_class.methods:
 #   print(m.qualified_name)

#for c in _globalAST.cxx_classes:
 #   print(c.qualified_name)

#print(cborData)


is_global_ast = sys.argv[1] == 'global.ast'
globalAST = GlobalAST()

if is_global_ast:
    print ("Opening global ast")

    globalAST.from_global_cbor(read_cbor(sys.argv[1]))
    if _check_sanity:
        globalAST.check_sanity()

else:
    print ("Processing files")
    localCborLoader = LocalCborLoader(globalAST, _check_sanity)
    local_cbor_files = sys.argv[1:]
    localCborLoader.load_local_cbor_from_files(local_cbor_files)
    globalAST.dump_global_cbor_to_file('global.ast')

#print(globalAST.function_map)


#print_calls(globalAST)

calls = calculate_call_counts(globalAST)
#print_signal_counts(globalAST)

#print("Printing QObjects")
#print_qobjects(_globalAST)

#for fun in calls:
    #print("{}: {} calls".format(fun[0].qualified_name, fun[1]))

unused_funs = filter(lambda x: x[1] == 0, calls)

unused_signals = filter(lambda x: isinstance(x[0], CXXMethod) and x[0].method_flags == 1, unused_funs)

print()
print("Unused signals:")

for fun in unused_signals:
    print(fun[0].qualified_name)
