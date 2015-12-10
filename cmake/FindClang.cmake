# Detect Clang libraries
#
# Defines the following variables:
#  CLANG_FOUND                 - True if Clang was found
#  CLANG_INCLUDE_DIR           - Where to find Clang includes
#  CLANG_LIBRARY_DIR           - Where to find Clang libraries
#
#  CLANG_LIBCLANG_LIB          - Libclang C library
#
#  CLANG_CLANGFRONTEND_LIB     - Clang Frontend Library
#  CLANG_CLANGDRIVER_LIB       - Clang Driver Library
#  ...
#
# Uses the same include and library paths detected by FindLLVM.cmake
#
# See http://clang.llvm.org/docs/InternalsManual.html for full list of libraries

#=============================================================================
# Copyright 2014-2015 Kevin Funk <kfunk@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.

#=============================================================================

if (${Clang_FIND_REQUIRED})
    find_package(LLVM ${Clang_FIND_VERSION} REQUIRED)
else ()
    find_package(LLVM ${Clang_FIND_VERSION})
endif ()

set(CLANG_FOUND FALSE)

if (LLVM_FOUND AND LLVM_LIBRARY_DIR)
  macro(FIND_AND_ADD_CLANG_LIB _libname_)
    string(TOUPPER ${_libname_} _prettylibname_)
    find_library(CLANG_${_prettylibname_}_LIB NAMES ${_libname_} HINTS ${LLVM_LIBRARY_DIR})
    if(CLANG_${_prettylibname_}_LIB)
      set(CLANG_LIBS ${CLANG_LIBS} ${CLANG_${_prettylibname_}_LIB})
    endif()
  endmacro(FIND_AND_ADD_CLANG_LIB)

  # note: On Windows there's 'libclang.dll' instead of 'clang.dll' -> search for 'libclang', too
  find_library(CLANG_LIBCLANG_LIB NAMES clang libclang HINTS ${LLVM_LIBRARY_DIR}) # LibClang: high-level C interface

  FIND_AND_ADD_CLANG_LIB(clangFrontend)
  FIND_AND_ADD_CLANG_LIB(clangDriver)
  FIND_AND_ADD_CLANG_LIB(clangCodeGen)
  FIND_AND_ADD_CLANG_LIB(clangSema)
  FIND_AND_ADD_CLANG_LIB(clangChecker)
  FIND_AND_ADD_CLANG_LIB(clangAnalysis)
  FIND_AND_ADD_CLANG_LIB(clangRewriteFrontend)
  FIND_AND_ADD_CLANG_LIB(clangRewrite)
  FIND_AND_ADD_CLANG_LIB(clangAST)
  FIND_AND_ADD_CLANG_LIB(clangParse)
  FIND_AND_ADD_CLANG_LIB(clangLex)
  FIND_AND_ADD_CLANG_LIB(clangBasic)
  FIND_AND_ADD_CLANG_LIB(clangARCMigrate)
  FIND_AND_ADD_CLANG_LIB(clangEdit)
  FIND_AND_ADD_CLANG_LIB(clangFrontendTool)
  FIND_AND_ADD_CLANG_LIB(clangRewrite)
  FIND_AND_ADD_CLANG_LIB(clangSerialization)
  FIND_AND_ADD_CLANG_LIB(clangTooling)
  FIND_AND_ADD_CLANG_LIB(clangStaticAnalyzerCheckers)
  FIND_AND_ADD_CLANG_LIB(clangStaticAnalyzerCore)
  FIND_AND_ADD_CLANG_LIB(clangStaticAnalyzerFrontend)
  FIND_AND_ADD_CLANG_LIB(clangSema)
  FIND_AND_ADD_CLANG_LIB(clangRewriteCore)
endif()

if(CLANG_LIBS)
  set(CLANG_FOUND TRUE)
else()
  message(STATUS "Could not find any Clang libraries in ${LLVM_LIBRARY_DIR}")
endif()

if(CLANG_FOUND)
  set(CLANG_LIBRARY_DIR ${LLVM_LIBRARY_DIR})
  set(CLANG_INCLUDE_DIR ${LLVM_INCLUDE_DIR})

  # check whether llvm-config comes from an install prefix
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --src-root
    OUTPUT_VARIABLE _llvmSourceRoot
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(FIND "${LLVM_INCLUDE_DIR}" "${_llvmSourceRoot}" _llvmIsInstalled)
  if (NOT _llvmIsInstalled)
    message(STATUS "Detected that llvm-config comes from a build-tree, adding includes from source dir")
    list(APPEND CLANG_INCLUDE_DIR "${_llvmSourceRoot}/tools/clang/include")
  endif()

  message(STATUS "Found Clang (LLVM version: ${LLVM_VERSION})")
  message(STATUS "  Include dirs:       ${CLANG_INCLUDE_DIR}")
  message(STATUS "  Clang libraries:    ${CLANG_LIBS}")
  message(STATUS "  Libclang C library: ${CLANG_LIBCLANG_LIB}")
else()
  if(Clang_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find Clang")
  endif()
endif()
