# Find the native LLVM includes and libraries
#
# Defines the following variables
#  LLVM_INCLUDE_DIRS   - where to find llvm include files
#  LLVM_LIBRARY_DIRS   - where to find llvm libs
#  LLVM_CFLAGS         - llvm compiler flags
#  LLVM_LFLAGS         - llvm linker flags
#  LLVM_MODULE_LIBS    - list of llvm libs for working with modules.
#  LLVM_INSTALL_PREFIX - LLVM installation prefix
#  LLVM_FOUND          - True if llvm found.
#  LLVM_VERSION        - Version string ("llvm-config --version")
#
# This module reads hints about search locations from variables
#  LLVM_ROOT           - Preferred LLVM installation prefix (containing bin/, lib/, ...)
#
#  Note: One may specify these as environment variables if they are not specified as
#   CMake variables or cache entries.

#=============================================================================
# SPDX-FileCopyrightText: 2014 Kevin Funk <kfunk@kde.org>
# SPDX-FileCopyrightText: 2024 Shivan Kunwar <shivam.kunwar@kdab.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#=============================================================================

if (NOT LLVM_ROOT AND DEFINED ENV{LLVM_ROOT})
    file(TO_CMAKE_PATH "$ENV{LLVM_ROOT}" LLVM_ROOT)
endif()

# if the user specified LLVM_ROOT, use that and fail otherwise
if (LLVM_ROOT)
  find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config HINTS ${LLVM_ROOT}/bin DOC "llvm-config executable" NO_DEFAULT_PATH)
else()
  # find llvm-config, prefer the one with a version suffix, e.g. llvm-config-14
  string(REPLACE "." "" LLVM_FIND_VERSION_CONCAT ${LLVM_FIND_VERSION})
  find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config-${LLVM_FIND_VERSION} llvm-config${LLVM_FIND_VERSION_CONCAT} llvm-config-14 llvm-config-15 llvm-config-16 llvm-config-17 llvm-config-18 llvm-config-19 llvm-config DOC "llvm-config executable")

  if (NOT LLVM_CONFIG_EXECUTABLE)
    find_program(_llvmNmExecutable llvm-nm)
    if (_llvmNmExecutable)
      execute_process(COMMAND ${_llvmNmExecutable} --version OUTPUT_VARIABLE _out)
      string(REGEX REPLACE ".*LLVM version ([^ \n]+).*" "\\1" _versionString "${_out}")
      find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config-${_versionString} DOC "llvm-config executable")
    endif()
  endif()
endif()

set(LLVM_FOUND FALSE)

if (LLVM_CONFIG_EXECUTABLE)
  # verify that we've found the correct version of llvm-config
  execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE LLVM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (NOT LLVM_VERSION)
    set(_LLVM_ERROR_MESSAGE "Failed to parse version from llvm-config")
  elseif (LLVM_VERSION VERSION_LESS "14.0")
    set(_LLVM_ERROR_MESSAGE "LLVM version too old: ${LLVM_VERSION}")
  else()
    set(LLVM_FOUND TRUE)
  endif()
else()
  set(_LLVM_ERROR_MESSAGE "Could NOT find 'llvm-config' executable")
endif()

if (LLVM_FOUND)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE LLVM_LIBRARY_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
    OUTPUT_VARIABLE LLVM_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
    OUTPUT_VARIABLE LLVM_LFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs core bitreader asmparser analysis
    OUTPUT_VARIABLE LLVM_MODULE_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE " " ";" LLVM_LIBS ${LLVM_LIBS})

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --prefix
    OUTPUT_VARIABLE LLVM_INSTALL_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --bindir
    OUTPUT_VARIABLE LLVM_BIN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(CLANG_EXECUTABLE_PATH "${LLVM_BIN_DIR}/clang")
  if(EXISTS ${CLANG_EXECUTABLE_PATH})
    message(STATUS "Clang found: ${CLANG_EXECUTABLE_PATH}")
    set(CLANG_EXECUTABLE ${CLANG_EXECUTABLE_PATH})
  else()
    message(WARNING "Clang not found in LLVM bin directory: ${LLVM_BIN_DIR}")
  endif()

  # potentially add include dir from binary dir for non-installed LLVM
  set(LLVM_CONFIG_INCLUDE_FLAG "--src-root")
  if (${LLVM_VERSION} VERSION_GREATER_EQUAL 16)
    set(LLVM_CONFIG_INCLUDE_FLAG "--includedir")
  endif()
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} ${LLVM_CONFIG_INCLUDE_FLAG}
    OUTPUT_VARIABLE _llvmSourceRoot
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(FIND "${LLVM_INCLUDE_DIRS}" "${_llvmSourceRoot}" _llvmIsInstalled)
  if (NOT _llvmIsInstalled)
    list(APPEND LLVM_INCLUDE_DIRS "${LLVM_INSTALL_PREFIX}/include")
  endif()

  # LLVM 19 specific handling
  if (LLVM_VERSION VERSION_GREATER_EQUAL "19.0")
    execute_process(
      COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs all
      OUTPUT_VARIABLE LLVM_LIBS_19
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE " " ";" LLVM_LIBS_19 ${LLVM_LIBS_19})
    list(APPEND LLVM_LIBS ${LLVM_LIBS_19})
  endif()
endif()

if (LLVM_FIND_REQUIRED AND NOT LLVM_FOUND)
  message(FATAL_ERROR "Could not find LLVM: ${_LLVM_ERROR_MESSAGE}")
elseif(_LLVM_ERROR_MESSAGE)
  message(STATUS "Could not find LLVM: ${_LLVM_ERROR_MESSAGE}")
endif()

if (LLVM_FOUND)
  message(STATUS "Found LLVM (version: ${LLVM_VERSION}): (using ${LLVM_CONFIG_EXECUTABLE})")
  message(STATUS "  Include dirs:   ${LLVM_INCLUDE_DIRS}")
  message(STATUS "  LLVM libraries: ${LLVM_LIBS}")
  message(STATUS "  LLVM System libraries: ${LLVM_SYSTEM_LIBS}")

  if (LLVM_VERSION VERSION_GREATER_EQUAL "19.0")
    message(STATUS "  LLVM 19+ detected, additional libraries may be available")
  endif()
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE LLVM_DEFINITIONS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX REPLACE "[ \t]+" ";" LLVM_DEFINITIONS "${LLVM_DEFINITIONS}")

if(LLVM_FIND_COMPONENTS)
  foreach(component ${LLVM_FIND_COMPONENTS})
    string(TOUPPER ${component} component_upcase)
    set(LLVM_${component_upcase}_FOUND TRUE)
    set(LLVM_${component_upcase}_LIBRARY ${component})
  endforeach()
endif()

if (LLVM_VERSION VERSION_GREATER_EQUAL "14.0")
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --has-rtti
    OUTPUT_VARIABLE LLVM_HAS_RTTI
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (LLVM_HAS_RTTI)
    message(STATUS "  LLVM built with RTTI: ${LLVM_HAS_RTTI}")
  endif()
endif()

set(LLVM_VERSION "${LLVM_VERSION}" CACHE STRING "LLVM version")

mark_as_advanced(LLVM_INCLUDE_DIRS LLVM_LIBRARY_DIRS LLVM_CFLAGS LLVM_LFLAGS LLVM_MODULE_LIBS LLVM_LIBS LLVM_DEFINITIONS)
