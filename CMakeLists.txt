# This is the top-level CMakeLists.txt file for the Clazy project.
#
# Pass the following variables to cmake to control the build:
#
# -DCLAZY_ON_WINDOWS_HACK=[true|false]
#  Use Windows plugin support hack to support plugins on Windows.
#  Default=false
#
project(ClangLazy)

cmake_minimum_required(VERSION 3.0)
include(GenerateExportHeader)
include(ClazySources.cmake)

# Version setup
set(CLAZY_VERSION_MAJOR "1")
set(CLAZY_VERSION_MINOR "1")
set(CLAZY_VERSION_PATCH "0")
set(CLAZY_VERSION "${CLAZY_VERSION_MAJOR}.${CLAZY_VERSION_MINOR}.${CLAZY_VERSION_PATCH}")
set(CLAZY_PRINT_VERSION "${CLAZY_VERSION_MAJOR}.${CLAZY_VERSION_MINOR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
find_package(Clang 3.6 MODULE REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions(-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

option(CLAZY_INSTALL_NO_HEADERS "Enable this option to suppress installation of headers" OFF)
option(CLAZY_ON_WINDOWS_HACK "Enable this option if you're using a patched clang to support plugins on Windows" OFF)
if(CLAZY_ON_WINDOWS_HACK)
  message("Using Windows plugin support hack")
  add_definitions(-DCLAZY_ON_WINDOWS_HACK)
endif()

if(MSVC)
  # disable trigger-happy warnings from Clang/LLVM headers
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4244 /wd4291 /wd4800 /wd4141 /wd4146")
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-common -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -fno-exceptions -fno-rtti")
endif()

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Look for std::regex support
message("Looking for std::regex support...")
try_run(RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake_has_regex_test.cpp)

if(RUN_RESULT EQUAL 0)
  set(HAS_STD_REGEX TRUE)
else()
  set(HAS_STD_REGEX FALSE)
endif()

string(REPLACE " " ";" LLVM_LIBS_LIST ${LLVM_LIBS}) # Transform into a list

if(NOT APPLE)
  # Don't link against LLVMSupport, causes: CommandLine Error: Option 'view-background' registered more than once!
  list(REMOVE_ITEM LLVM_LIBS_LIST "-lLLVMSupport")    # Remove element
endif()

macro(link_to_llvm name)
  foreach(clang_lib ${CLANG_LIBS})
    target_link_libraries(${name} ${clang_lib})
  endforeach()

  foreach(llvm_lib ${LLVM_LIBS_LIST})
    target_link_libraries(${name} ${llvm_lib})
  endforeach()

  foreach(user_lib ${USER_LIBS})
    target_link_libraries(${name} ${user_lib})
  endforeach()

  foreach(llvm_system_lib ${LLVM_SYSTEM_LIBS})
    target_link_libraries(${name} ${llvm_system_lib})
  endforeach()

endmacro()

macro(add_clang_plugin name)
  set(srcs ${ARGN})

  include_directories(${CLANG_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
  link_directories("${LLVM_INSTALL_PREFIX}/lib" ${LLVM_LIBRARY_DIRS})

  add_library(${name} SHARED ${srcs})

  if(SYMBOL_FILE)
    set_target_properties(${name} PROPERTIES LINK_FlAGS "-exported_symbols_list ${SYMBOL_FILE}")
  endif()

  link_to_llvm(${name})

  if(MSVC)
    target_link_libraries(${name} version.lib)
  endif()

  target_link_libraries(${name} clazylib)
endmacro()

# clazylib version
set(clazylib_VERSION_MAJOR 0)
set(clazylib_VERSION_MINOR 1)
# Enable the full x.y.z version only for release versions
set(clazylib_VERSION_PATCH 0)
set(clazylib_VERSION ${clazylib_VERSION_MAJOR}.${clazylib_VERSION_MINOR})

include_directories(${CMAKE_BINARY_DIR})

link_directories("${LLVM_INSTALL_PREFIX}/lib" ${LLVM_LIBRARY_DIRS})
add_library(clazylib SHARED ${CLAZY_LIB_SRC})
link_to_llvm(clazylib)

generate_export_header(clazylib)

set_target_properties(clazylib PROPERTIES VERSION ${clazylib_VERSION} SOVERSION ${clazylib_VERSION_MAJOR})
include("GNUInstallDirs")

install(TARGETS clazylib EXPORT LibClaryExport
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(SYMBOL_FILE Lazy.exports)

if(HAS_STD_REGEX)
  #level2
  set(SRC_FILES ${SRC_FILES} checks/level2/oldstyleconnect.cpp)
else()
  add_definitions(-DNO_STD_REGEX)
  message("old-style-connect check is disabled due to missing std::regex support")
  message("Suppressions are disabled due to missing std::regex support")
endif()

add_clang_plugin(ClangLazy ${SRC_FILES})

set_target_properties(ClangLazy PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX ""
)

install(TARGETS ClangLazy
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(SHARE_INSTALL_DIR ${CMAKE_INSTALL_DATAROOTDIR} CACHE STRING "Share directory name")

if(NOT WIN32)
  configure_file(${CMAKE_SOURCE_DIR}/clazy.cmake ${CMAKE_BINARY_DIR}/clazy @ONLY)
  install(FILES ${CMAKE_BINARY_DIR}/clazy DESTINATION bin PERMISSIONS OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
else()
  if(MSVC)
    set(CLANG_COMPILER "clang-cl")
  else()
    set(CLANG_COMPILER "clang++")
  endif()
  configure_file(${CMAKE_SOURCE_DIR}/clazy.bat.cmake ${CMAKE_BINARY_DIR}/clazy.bat)
  install(FILES ${CMAKE_BINARY_DIR}/clazy.bat DESTINATION bin PERMISSIONS OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
endif()

# Install the explanation README's
set(DOC_INSTALL_DIR ${SHARE_INSTALL_DIR}/clazy/doc)
file(GLOB README_LEVEL0_FILES checks/level0/README-*)
file(GLOB README_LEVEL1_FILES checks/level1/README-*)
file(GLOB README_LEVEL2_FILES checks/level2/README-*)
file(GLOB README_LEVEL3_FILES checks/level3/README-*)
file(GLOB README_HIDDENLEVEL_FILES checks/hiddenlevel/README-*)
install(FILES ${README_LEVEL0_FILES} DESTINATION ${DOC_INSTALL_DIR}/level0)
install(FILES ${README_LEVEL1_FILES} DESTINATION ${DOC_INSTALL_DIR}/level1)
install(FILES ${README_LEVEL2_FILES} DESTINATION ${DOC_INSTALL_DIR}/level2)
install(FILES ${README_LEVEL3_FILES} DESTINATION ${DOC_INSTALL_DIR}/level3)
install(FILES ${README_HIDDENLEVEL_FILES} DESTINATION ${DOC_INSTALL_DIR}/hiddenlevel)

# Install more doc files
install(FILES README.md LICENSE.GPL.txt DESTINATION ${DOC_INSTALL_DIR})

if (NOT CLAZY_INSTALL_NO_HEADERS)
  # Install public headers
  set(CLAZY_LIB_INCLUDES
    AccessSpecifierManager.h
    checkbase.h
    checkmanager.h
    ${CMAKE_BINARY_DIR}/clazylib_export.h
    clazy_stl.h
    ContextUtils.h
    FixItUtils.h
    HierarchyUtils.h
    LoopUtils.h
    MacroUtils.h
    QtUtils.h
    SuppressionManager.h
    StmtBodyRange.h
    StringUtils.h
    TemplateUtils.h
    TypeUtils.h
    Utils.h
  )
  install(FILES ${CLAZY_LIB_INCLUDES} DESTINATION include/clazy)
endif()
