# This is the top-level CMakeLists.txt file for the Clazy project.
#
# To build the man page from POD, run 'make man' after CMake (assumes perl is available)
# To install the resulting man page, run 'make install'
# The man page is not available on Windows.
#

project(clazy)

cmake_minimum_required(VERSION 3.3)
include(FeatureSummary)
include(GenerateExportHeader)
include("GNUInstallDirs")

# Version setup
set(CLAZY_VERSION_MAJOR "1")
set(CLAZY_VERSION_MINOR "6")
set(CLAZY_VERSION_PATCH "0")
set(CLAZY_VERSION "${CLAZY_VERSION_MAJOR}.${CLAZY_VERSION_MINOR}.${CLAZY_VERSION_PATCH}")
set(CLAZY_PRINT_VERSION "${CLAZY_VERSION_MAJOR}.${CLAZY_VERSION_MINOR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
if (NOT CLAZY_BUILD_WITH_CLANG)
  find_package(Clang 4.0 MODULE REQUIRED)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions(-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

option(CLAZY_AST_MATCHERS_CRASH_WORKAROUND "Disable AST Matchers if being built with clang. See bug #392223" ON)
option(LINK_CLAZY_TO_LLVM "Links the clazy plugin to LLVM. Switch to OFF if your clang binary has all symbols already. Might need to be OFF if your LLVM is static." ON)

if (CLAZY_AST_MATCHERS_CRASH_WORKAROUND AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message("Enabling AST Matchers workaround. Consider building with gcc instead. See bug #392223.")
    add_definitions(-DCLAZY_DISABLE_AST_MATCHERS)
endif()

if(NOT CLAZY_BUILD_WITH_CLANG AND MSVC AND NOT CLANG_LIBRARY_IMPORT)
  message(FATAL_ERROR "\nOn MSVC you need to pass -DCLANG_LIBRARY_IMPORT=C:/path/to/llvm-build/lib/clang.lib to cmake when building Clazy.\nAlso make sure you've built LLVM with -DLLVM_EXPORT_SYMBOLS_FOR_PLUGINS=ON")
endif()

if(MSVC)
  # disable trigger-happy warnings from Clang/LLVM headers
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4244 /wd4291 /wd4800 /wd4141 /wd4146 /wd4251")
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-common -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -fno-exceptions -fno-rtti")
endif()

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Look for std::regex support
message("Looking for std::regex support...")
try_run(RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/.cmake_has_regex_test.cpp)

if(RUN_RESULT EQUAL 0)
  set(HAS_STD_REGEX TRUE)
else()
  set(HAS_STD_REGEX FALSE)
  add_definitions(-DNO_STD_REGEX)
  message("old-style-connect check is disabled due to missing std::regex support")
  message("Suppressions are disabled due to missing std::regex support")
endif()

include(ClazySources.cmake)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CLANG_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src)
link_directories("${LLVM_INSTALL_PREFIX}/lib" ${LLVM_LIBRARY_DIRS})

macro(link_to_llvm name is_standalone)
  foreach(clang_lib ${CLANG_LIBS})
    if(MSVC)
      get_filename_component(LIB_FILENAME ${clang_lib} NAME)
      if(LIB_FILENAME STREQUAL "clangFrontend.lib")
        # On MSVC we don't link against clangFrontend.lib, instead we link against clang.exe (via clang.lib)
        # Otherwise the clazy plugin would have it's own plugin registry and clang wouldn't see it.
        # This way clazy registers with clang.
        continue()
      endif()
    endif()

    target_link_libraries(${name} ${clang_lib})
  endforeach()

  foreach(llvm_lib ${LLVM_LIBS})
    if(NOT ${is_standalone} AND NOT APPLE AND NOT MINGW AND NOT MSVC)
        ## Don't link against LLVMSupport, causes: CommandLine Error: Option 'view-background' registered more than once!
        if (NOT llvm_lib MATCHES ".*LLVMSupport.*")
            target_link_libraries(${name} ${llvm_lib})
        endif()
    else()
        target_link_libraries(${name} ${llvm_lib})
    endif()
  endforeach()

  foreach(user_lib ${USER_LIBS})
    target_link_libraries(${name} ${user_lib})
  endforeach()

  foreach(llvm_system_lib ${LLVM_SYSTEM_LIBS})
    target_link_libraries(${name} ${llvm_system_lib})
  endforeach()

  if(WIN32)
    target_link_libraries(${name} version.lib)
  endif()
endmacro()

macro(add_clang_plugin name)
  set(srcs ${ARGN})

  add_library(${name} SHARED ${srcs})

  if(SYMBOL_FILE)
    set_target_properties(${name} PROPERTIES LINK_FlAGS "-exported_symbols_list ${SYMBOL_FILE}")
  endif()

  if (LINK_CLAZY_TO_LLVM)
    link_to_llvm(${name} FALSE)
  endif()

  if(MSVC)
    target_link_libraries(${name} ${CLANG_LIBRARY_IMPORT}) # Link against clang.exe to share the plugin registry
  endif()

endmacro()

set(SYMBOL_FILE Lazy.exports)

if (NOT CLAZY_BUILD_WITH_CLANG)
  add_clang_plugin(ClazyPlugin ${CLAZY_PLUGIN_SRCS})
  set_target_properties(ClazyPlugin PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX ""
  )

  install(TARGETS ClazyPlugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  set(SHARE_INSTALL_DIR ${CMAKE_INSTALL_DATAROOTDIR} CACHE STRING "Share directory name")

  if(NOT WIN32)
    file(RELATIVE_PATH BIN_RELATIVE_LIBDIR "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")
    file(RELATIVE_PATH BIN_RELATIVE_SHAREDIR "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_DATAROOTDIR}")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/clazy.cmake ${CMAKE_BINARY_DIR}/clazy @ONLY)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/clazy DESTINATION bin)
  else()
    install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/clazy.bat DESTINATION bin)
    if(MSVC)
      install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/clazy-cl.bat DESTINATION bin)
    endif()
  endif()

  # Install the explanation README's
  set(DOC_INSTALL_DIR ${SHARE_INSTALL_DIR}/doc/clazy)

  include(${CMAKE_CURRENT_LIST_DIR}/readmes.cmake)

  install(FILES ${README_LEVEL0_FILES} DESTINATION ${DOC_INSTALL_DIR}/level0)
  install(FILES ${README_LEVEL1_FILES} DESTINATION ${DOC_INSTALL_DIR}/level1)
  install(FILES ${README_LEVEL2_FILES} DESTINATION ${DOC_INSTALL_DIR}/level2)
  install(FILES ${README_LEVEL3_FILES} DESTINATION ${DOC_INSTALL_DIR}/level3)
  install(FILES ${README_manuallevel_FILES} DESTINATION ${DOC_INSTALL_DIR}/manuallevel)

  # Install more doc files
  install(FILES README.md COPYING-LGPL2.txt checks.json DESTINATION ${DOC_INSTALL_DIR})

  # Build docs
  set(MAN_INSTALL_DIR "${SHARE_INSTALL_DIR}/man/man1")
  add_subdirectory(docs)

  # rpath
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  endif("${isSystemDir}" STREQUAL "-1")

  # Build clazy-standalone
  add_executable(clazy-standalone ${CLAZY_STANDALONE_SRCS})

  if(MSVC)
    # On MSVC clang-standalone crashes with a meaningless backtrace if linked to ClazyPlugin.dll
    target_link_libraries(clazy-standalone clangFrontend)
  else()
    target_link_libraries(clazy-standalone ClazyPlugin)
  endif()

  link_to_llvm(clazy-standalone TRUE)

  install(TARGETS clazy-standalone DESTINATION bin PERMISSIONS OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)

  set(CPACK_PACKAGE_VERSION_MAJOR ${CLAZY_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${CLAZY_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${CLAZY_VERSION_PATCH})
  include(CPack)
else()
  set(LLVM_LINK_COMPONENTS
    Support
    )
  add_clang_library(clazyPlugin
    ${CLAZY_PLUGIN_SRCS}

    LINK_LIBS
    clangFrontend
    clangDriver
    clangCodeGen
    clangSema
    clangAnalysis
    clangRewriteFrontend
    clangRewrite
    clangAST
    clangASTMatchers
    clangParse
    clangLex
    clangBasic
    clangARCMigrate
    clangEdit
    clangFrontendTool
    clangRewrite
    clangSerialization
    clangTooling
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangStaticAnalyzerFrontend
    )
  add_executable(clazy-standalone ${CLAZY_STANDALONE_SRCS})

  target_link_libraries(clazy-standalone clazyPlugin)

  install(TARGETS clazy-standalone DESTINATION bin PERMISSIONS OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
endif()
