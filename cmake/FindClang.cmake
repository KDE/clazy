# Detect Clang libraries
#
# Defines the following variables:
#  CLANG_FOUND                 - True if Clang was found
#  CLANG_INCLUDE_DIR           - Where to find Clang includes
#  CLANG_LIBRARY_DIR           - Where to find Clang libraries
#
#  CLANG_CLANG_LIB             - LibClang library
#  CLANG_CLANGFRONTEND_LIB     - Clang Frontend Library
#  CLANG_CLANGDRIVER_LIB       - Clang Driver Library
#  ...
#
# Uses the same include and library paths detected by FindLLVM.cmake
#
# See http://clang.llvm.org/docs/InternalsManual.html for full list of libraries

#=============================================================================
# Copyright 2014 Kevin Funk <kfunk@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.

#=============================================================================


# WARNING: This file was copied from kdevelop/languages/clang/cmake/FindClang.cmake
# submit any patches there instead

if(${Clang_FIND_REQUIRED})
  set(LLVM_FIND_TYPE REQUIRED)
else()
  set(LLVM_FIND_TYPE OPTIONAL)
endif()

find_package(LLVM CONFIG ${LLVM_FIND_TYPE} QUIET)
# if(NOT LLVM_FOUND)
#   find_package(LLVM 3.4 CONFIG ${LLVM_FIND_TYPE} QUIET)
# endif()
# if(NOT LLVM_FOUND)
#   find_package(LLVM 3.5 CONFIG ${LLVM_FIND_TYPE} QUIET)
# endif()
# if(NOT LLVM_FOUND)
#   find_package(LLVM 3.6 CONFIG ${LLVM_FIND_TYPE} QUIET)
# endif()

set(Clang_FOUND FALSE)

set(CLANG_LIBRARY_DIR ${LLVM_LIBRARY_DIRS})
set(CLANG_INCLUDE_DIR ${LLVM_INCLUDE_DIRS})

macro(FIND_AND_ADD_CLANG_LIB _libname_)
  string(TOUPPER ${_libname_} _prettylibname_)
  find_library(CLANG_${_prettylibname_}_LIB NAMES "clang${_libname_}" HINTS ${LLVM_LIBRARY_DIRS})
  if(CLANG_${_prettylibname_}_LIB)
    add_library(Clang::${_libname_} UNKNOWN IMPORTED)
    set_property(TARGET Clang::${_libname_} PROPERTY IMPORTED_LOCATION "${CLANG_${_prettylibname_}_LIB}")
    set_property(TARGET Clang::${_libname_} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CLANG_INCLUDE_DIR})

    set(CLANG_LIBS ${CLANG_LIBS} ${CLANG_${_prettylibname_}_LIB})
  endif()
endmacro(FIND_AND_ADD_CLANG_LIB)

FIND_AND_ADD_CLANG_LIB(Frontend)
FIND_AND_ADD_CLANG_LIB(Driver)
FIND_AND_ADD_CLANG_LIB(CodeGen)
FIND_AND_ADD_CLANG_LIB(Sema)
FIND_AND_ADD_CLANG_LIB(Checker)
FIND_AND_ADD_CLANG_LIB(Analysis)
FIND_AND_ADD_CLANG_LIB(Rewrite)
FIND_AND_ADD_CLANG_LIB(AST)
FIND_AND_ADD_CLANG_LIB(Parse)
FIND_AND_ADD_CLANG_LIB(Lex)
FIND_AND_ADD_CLANG_LIB(Basic)
FIND_AND_ADD_CLANG_LIB(ARCMigrate)
FIND_AND_ADD_CLANG_LIB(Edit)
FIND_AND_ADD_CLANG_LIB(FrontendTool)
FIND_AND_ADD_CLANG_LIB(Serialization)
FIND_AND_ADD_CLANG_LIB(Tooling)
FIND_AND_ADD_CLANG_LIB(ToolingCore)
FIND_AND_ADD_CLANG_LIB(StaticAnalyzerCheckers)
FIND_AND_ADD_CLANG_LIB(StaticAnalyzerCore)
FIND_AND_ADD_CLANG_LIB(StaticAnalyzerFrontend)
FIND_AND_ADD_CLANG_LIB(RewriteCore)
FIND_AND_ADD_CLANG_LIB(ASTMatchers)

if(CLANG_LIBS)
  set(Clang_FOUND TRUE)
  set(Clang_VERSION ${LLVM_VERSION})

else()
  message(STATUS "Could not find any Clang libraries in ${LLVM_LIBRARY_DIR}")
endif()

if(Clang_FOUND)
  message(STATUS "Found Clang (LLVM version: ${LLVM_VERSION})")
  message(STATUS "  Include dirs:  ${CLANG_INCLUDE_DIR}")
  message(STATUS "  Library dir:  ${CLANG_LIBRARY_DIR}")
  message(STATUS "  Libraries:     ${CLANG_LIBS}")

  string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_VERSION})
  list(GET LLVM_VERSION_LIST 0 LLVM_VERSION_MAJOR)
  list(GET LLVM_VERSION_LIST 1 LLVM_VERSION_MINOR)
  list(GET LLVM_VERSION_LIST 2 LLVM_VERSION_PATCH)
else()
  if(Clang_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find Clang")
  endif()
endif()
