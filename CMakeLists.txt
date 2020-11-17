# This is the top-level CMakeLists.txt file for the Clazy project.
#
# To build the man page from POD, run 'make man' after CMake (assumes perl is available)
# To install the resulting man page, run 'make install'
# The man page is not available on Windows.
#

project(clazy)

cmake_minimum_required(VERSION 3.7)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    cmake_policy(SET CMP0074 NEW)
endif()

include(FeatureSummary)
include(GenerateExportHeader)
include(GNUInstallDirs)

# Version setup
set(CLAZY_VERSION_MAJOR "1")
set(CLAZY_VERSION_MINOR "8")
set(CLAZY_VERSION_PATCH "0")
set(CLAZY_VERSION "${CLAZY_VERSION_MAJOR}.${CLAZY_VERSION_MINOR}.${CLAZY_VERSION_PATCH}")
set(CLAZY_PRINT_VERSION "${CLAZY_VERSION_MAJOR}.${CLAZY_VERSION_MINOR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
if (NOT CLAZY_BUILD_WITH_CLANG)
  find_package(Clang 5.0 MODULE REQUIRED)

  if (CLANG_CLANG-CPP_LIB AND NOT APPLE)
    set(default_use_clang_cpp ON)
  else()
    set(default_use_clang_cpp OFF)
  endif()
  option(CLAZY_LINK_CLANG_DYLIB "Link clazy to dynamic Clang C++ library clang-cpp." ${default_use_clang_cpp})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions(-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

option(CLAZY_AST_MATCHERS_CRASH_WORKAROUND "Disable AST Matchers if being built with clang. See bug #392223" ON)
option(LINK_CLAZY_TO_LLVM "Links the clazy plugin to LLVM. Switch to OFF if your clang binary has all symbols already. Might need to be OFF if your LLVM is static." ON)
option(APPIMAGE_HACK "Links the clazy plugin to the clang tooling libs only. For some reason this is needed when building on our old CentOS 6.8 to create the AppImage." OFF)

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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-common -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -fno-exceptions -fno-rtti")
endif()

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Look for std::regex support
message("Looking for std::regex support...")
try_run(REGEX_RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/.cmake_has_regex_test.cpp)

if(NOT REGEX_RUN_RESULT EQUAL 0)
  message("Using boost::regex instead of std::regex")
  set(CLAZY_USES_BOOST_REGEX TRUE)
  add_definitions(-DCLAZY_USES_BOOST_REGEX)
  find_package(Boost REQUIRED COMPONENTS regex)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# Compiler might support c++17 but not have std::filesystem in the standard library
message("Looking for std::filesystem support...")
try_run(FILESYSTEM_RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/.cmake_has_filesystem_test.cpp OUTPUT_VARIABLE foo CXX_STANDARD 17 CXX_STANDARD_REQUIRED true)

if (FILESYSTEM_RUN_RESULT EQUAL 0 AND COMPILE_RESULT)
    set(CMAKE_CXX_STANDARD 17)
    add_definitions(-DHAS_STD_FILESYSTEM)
endif()

include(ClazySources.cmake)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CLANG_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src)
link_directories("${LLVM_INSTALL_PREFIX}/lib" ${LLVM_LIBRARY_DIRS})

if (${LLVM_VERSION} VERSION_GREATER_EQUAL "9.0.0")
    set(clang_tooling_refactoring_lib clangToolingRefactoring)
else()
    set(clang_tooling_refactoring_lib clangToolingRefactor)
endif()

macro(link_to_llvm name is_standalone)
  if (CLAZY_LINK_CLANG_DYLIB)
    target_link_libraries(${name} clang-cpp)
  else()
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
    target_link_libraries(${name} clangTooling)
    target_link_libraries(${name} clangToolingCore)
    target_link_libraries(${name} ${clang_tooling_refactoring_lib})
  endif()

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
  if (CLAZY_USES_BOOST_REGEX)
    target_link_libraries(${name} ${Boost_LIBRARIES})
  endif()
endmacro()

macro(add_clang_plugin name)
  set(srcs ${ARGN})

  add_library(${name} SHARED ${srcs})

  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
      # 30% speedup
      target_precompile_headers(${name} PRIVATE src/checkbase.h)
  endif()

  if(SYMBOL_FILE)
    set_target_properties(${name} PROPERTIES LINK_FlAGS "-exported_symbols_list ${SYMBOL_FILE}")
  endif()

  if (LINK_CLAZY_TO_LLVM)
    link_to_llvm(${name} FALSE)
  else()
    if (APPIMAGE_HACK)
        # Hack to build on old CentOS 6.8
        target_link_libraries(${name} clangTooling)
        target_link_libraries(${name} clangToolingCore)
        target_link_libraries(${name} ${clang_tooling_refactoring_lib})
    endif()
  endif()

  if(MSVC)
    target_link_libraries(${name} ${CLANG_LIBRARY_IMPORT}) # Link against clang.exe to share the plugin registry
  endif()

endmacro()

set(SYMBOL_FILE Lazy.exports)

if (NOT CLAZY_BUILD_WITH_CLANG)
  set(CLAZY_MINI_AST_DUMPER_SRCS src/MiniAstDumper.cpp)
  add_clang_plugin(ClazyPlugin ${CLAZY_PLUGIN_SRCS} ${CLAZY_MINI_AST_DUMPER_SRCS})
  set_target_properties(ClazyPlugin PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX ""
  )

  install(TARGETS ClazyPlugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  if(NOT WIN32)
    if(APPLE)
      find_program(READLINK_CMD greadlink)
    else()
      find_program(READLINK_CMD readlink)
    endif()
    if(NOT READLINK_CMD)
      message(FATAL_ERROR "Could not find a proper readlink.  On Mac OSX you should install coreutils using homebrew in order to use the GNU readlink")
    endif()
    file(RELATIVE_PATH BIN_RELATIVE_LIBDIR "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")
    file(RELATIVE_PATH BIN_RELATIVE_SHAREDIR "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_DATAROOTDIR}")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/clazy.cmake ${CMAKE_BINARY_DIR}/clazy @ONLY)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/clazy DESTINATION bin)
  else()
    install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/clazy.bat DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(MSVC)
      install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/clazy-cl.bat DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
  endif()

  # Install the explanation README's
  include(${CMAKE_CURRENT_LIST_DIR}/readmes.cmake)

  install(FILES ${README_LEVEL0_FILES} DESTINATION ${CMAKE_INSTALL_DOCDIR}/level0)
  install(FILES ${README_LEVEL1_FILES} DESTINATION ${CMAKE_INSTALL_DOCDIR}/level1)
  install(FILES ${README_LEVEL2_FILES} DESTINATION ${CMAKE_INSTALL_DOCDIR}/level2)
  install(FILES ${README_manuallevel_FILES} DESTINATION ${CMAKE_INSTALL_DOCDIR}/manuallevel)

  # Install more doc files
  install(FILES README.md COPYING-LGPL2.txt checks.json DESTINATION ${CMAKE_INSTALL_DOCDIR})

  # Build docs
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
    clangToolingCore
    clangToolingInclusions
    ${clang_tooling_refactoring_lib}
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

function(to_raw_string_literal input_string output_string)
    if (MSVC)
        # Work around "C2026: string too big, trailing characters truncated"
        #   https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2026?view=vs-2019
        # The limit is 16380 single-byte characters, so split up the string as
        # suggested on the site.
        set(str ${input_string})
        set(chunk_size 1000)
        set(result "\n")
        string(LENGTH ${str} str_size)
        while (${str_size} GREATER ${chunk_size})
            string(SUBSTRING ${str} 0 ${chunk_size} chunk)
            string(SUBSTRING ${str} ${chunk_size} -1 str)
            set(chunk "R\"meta(${chunk})meta\"\n")
            string(APPEND result ${chunk})
            string(LENGTH ${str} str_size)
        endwhile()
        if (str_size GREATER 0)
            string(APPEND result "R\"meta(${str})meta\"\n")
        endif()
        set(${output_string} ${result} PARENT_SCOPE)
    else()
        set(result "\nR\"meta(${input_string})meta\"\n")
        set(${output_string} ${result} PARENT_SCOPE)
    endif()
endfunction()

file(READ checks.json SUPPORTED_CHECKS_JSON_STR)
to_raw_string_literal(${SUPPORTED_CHECKS_JSON_STR} SUPPORTED_CHECKS_JSON_STR)
configure_file(checks.json.h.in checks.json.h)

install(FILES org.kde.clazy.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
