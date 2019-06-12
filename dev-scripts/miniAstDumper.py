#!/usr/bin/env python

import cbor, sys, time, os

ClassFlag_None = 0
ClassFlag_QObject = 1

MethodFlag_None = 0
MethodFlag_Signal = 1

class SourceLocation:
    def __init__(self):
        self.filename = ""
        self.lineNumber = -1
        self.columnNumber = -1
        self.spellingFilename = ""
        self.spellingLineNumber = -1
        self.spellingColumnNumber = -1

    def asString(self):
        result = self.filename + ":" + str(self.lineNumber) + ":" + str(self.columnNumber)
        if self.spellingFilename: # if the loc is the same then the spelling filename is the same in both, so don't add it. They only diverge on line/column, as a macro can't be defined in two files
            result += ":" + str(self.spellingLineNumber) + ":" + str(self.spellingColumnNumber)
        return result

    def dump(self):
        print(self.asString())

    def check_sanity(self):
        if self.lineNumber < 0 or self.columnNumber < 0:
            print("SourceLocation::check_sanity: loc numbers are invalid! " + self.lineNumber + " ; " + self.columnNumber)

        if not self.filename:
            print("SourceLocation::check_sanity: loc filename is invalid!")

class FunctionCall:
    def __init__(self):
        self.callee_id = -1
        self.loc_start = SourceLocation()

    def check_sanity(self):
        if self.callee_id == -1:
            print("FunctionCall::check_sanity: callee_id is -1!")
        self.loc_start.check_sanity()

class Function:
    def __init__(self):
        self.id = 0
        self.qualified_name = ""
        self.num_called = 0 # How many times this function was called

    def check_sanity(self):
        if self.id == -1:
            print("CXXMethod::check_sanity: id is -1!")

        if not self.qualified_name:
            print("CXXMethod::check_sanity: qualified_name is empty!")

class CXXMethod(Function):
    def __init__(self):
        Function.__init__(self)
        self.method_flags = 0

class CXXClass:
    def __init__(self):
        # self.id = -1
        self.qualified_name = ""
        self.loc_start = SourceLocation()
        self.methods = []
        self.class_flags = 0

    def check_sanity(self):
        #if self.id == -1:
        #    print("CXXClass::check_sanity: id is -1!")

        if not self.qualified_name:
            print("CXXClass::check_sanity: qualified_name is empty!")

        for m in self.methods:
            m.check_sanity()

    def hash(self):
        return self.qualified_name + self.loc_start.asString()

class GlobalAST:
    def __init__(self):
        self.cxx_classes = {} # indexed by hash
        self.functions = []
        self.function_calls = []


    def check_sanity(self):
        for c in self.cxx_classes.values():
            c.check_sanity()
        for f in self.function_calls:
            f.check_sanity()


_check_sanity = False
_globalAST = GlobalAST()
_next_function_id = 1
_function_map = {}

def next_function_id():
    global _next_function_id
    result = _next_function_id
    _next_function_id += 1
    return result

def absolute_file(filename, tu_cwd):
    if not filename.startswith('/'): # TODO windows
        filename = tu_cwd + '/' + filename

    # Normalize
    filename = os.path.normpath(filename)
    return filename

def parse_loc(cborLoc, file_map, tu_cwd):
    loc = SourceLocation()
    loc.filename = file_map[str(cborLoc['fileId'])]
    loc.filename = absolute_file(loc.filename, tu_cwd)

    loc.lineNumber = cborLoc['line']
    loc.columnNumber = cborLoc['column']

    if 'spellingFileId' in cborLoc:
        loc.spellingFilename = file_map[str(cborLoc['spellingFileId'])]
        loc.lineNumber = cborLoc['spellingLineNumber']
        loc.columnNumber = cborLoc['spellingColumnNumber']

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

    tu_function_map = {}


    # populate the file map
    if 'files' in cborData:
        for fileId in cborData['files'].keys():
            file_map[fileId] = cborData['files'][fileId]


    if 'stuff' in cborData:
        # Process classes and methods
        for stuff in cborData['stuff']:
            if 'type' in stuff:
                if stuff['type'] == 31: # CXXRecordDecl
                    cxxclass = CXXClass()
                    # cxxclass.id = stuff['id']
                    cxxclass.qualified_name = stuff['name']
                    if 'class_flags' in stuff:
                        cxxclass.class_flags = stuff['class_flags']

                    cxxclass.loc_start = parse_loc(stuff['loc'], file_map, current_tu_cwd)

                    if 'methods' in stuff:
                        for m in stuff['methods']:
                            method = CXXMethod()
                            method.id = next_function_id() # Attribute a sequential id, that's unique across TUs
                            _function_map[method.id] = method

                            method.qualified_name = m['name']
                            cxxclass.methods.append(method)

                            if 'method_flags' in m:
                                method.method_flags = m['method_flags']

                            local_id_in_tu = m['id']

                            if local_id_in_tu in tu_function_map.keys():
                                print('method is duplicated! ' + method.qualified_name)
                            tu_function_map[local_id_in_tu] = method

                    globalAST.cxx_classes[cxxclass.hash()] = cxxclass

                if stuff['type'] == 48: # FunctionDecl
                    func = Function()
                    func.id = next_function_id() # Attribute a sequential id, that's unique across TUs
                    _function_map[func.id] = func
                    func.qualified_name = stuff['name']
                    globalAST.functions.append(func)
                    local_id_in_tu = stuff['id']
                    if local_id_in_tu in tu_function_map.keys():
                        print('function is duplicated! ' + method.qualified_name)
                    tu_function_map[local_id_in_tu] = func

                    if func.qualified_name == 'qBound':
                        print("qBound has id=" + str(local_id_in_tu))


        # Process CallExprs
        for stuff in cborData['stuff']:
            if 'stmt_type' in stuff and stuff['stmt_type'] == 48: # CallExpr
                funccall = FunctionCall()
                local_callee_id_in_tu = stuff['calleeId']
                source_loc = parse_loc(stuff['loc'], file_map, current_tu_cwd);

                if local_callee_id_in_tu not in tu_function_map.keys():
                    print("Could not find function with local tu id=" + str(local_callee_id_in_tu) + ", loc=" + source_loc.asString())
                else:
                    method = tu_function_map[local_callee_id_in_tu]
                    funccall.callee_id = method.id
                    funccall.loc_start = source_loc
                    globalAST.function_calls.append(funccall)


def get_all_method_names():
    total_methods = []
    for c in _globalAST.cxx_classes.values():
        for m in c.methods:
            total_methods.append(m.qualified_name)
    return total_methods

def print_qobjects(ast):
    for c in ast.cxx_classes.values():
        if c.class_flags & ClassFlag_QObject:
            print(c.qualified_name)


def calculate_call_counts(ast):
    for c in ast.function_calls:
        if c.callee_id in _function_map:
            _function_map[c.callee_id].num_called += 1

def print_unused_signals(ast):
    for c in ast.cxx_classes.values():
        for m in c.methods:
            if (m.method_flags & MethodFlag_Signal) and m.num_called == 0:
                print("Signal " + m.qualified_name + " never called")



def process_all_files(files):
    for f in files:
        load_cbor(f, _globalAST)

    return True



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




# load_cbor(sys.argv[1], _globalAST)


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


if not process_all_files(sys.argv[1:]):
    print('Error processing files')
    sys.exit(1)

#calculate_call_counts(_globalAST)
#print_unused_signals(_globalAST)
print_qobjects(_globalAST)
