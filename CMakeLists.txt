cmake_minimum_required (VERSION 2.6)
project (ClangLazy)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
find_package(Clang 3.6 MODULE REQUIRED)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

add_definitions (-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions (-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

OPTION(CLAZY_ON_WINDOWS_HACK "Enable this option if you're using a patched clang to support plugins on Windows" OFF)
if(CLAZY_ON_WINDOWS_HACK)
  message("Using Windows plugin support hack")
  ADD_DEFINITIONS(-DCLAZY_ON_WINDOWS_HACK)
endif(CLAZY_ON_WINDOWS_HACK)


if(MSVC)
    # disable trigger-happy warnings from Clang/LLVM headers
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4244 /wd4291 /wd4800")
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-common -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -fno-exceptions -fno-rtti")
endif()
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
if(NOT WIN32)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Look for std::regex support
message("Looking for std::regex support...")
try_run(RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake_has_regex_test.cpp)

if (RUN_RESULT EQUAL 0)
    set(HAS_STD_REGEX TRUE)
else()
    set(HAS_STD_REGEX FALSE)
endif()

# Don't link against LLVMSupport, causes: CommandLine Error: Option 'view-background' registered more than once!
STRING(REPLACE " " ";" LLVM_LIBS_LIST ${LLVM_LIBS}) # Transform into a list
LIST(REMOVE_ITEM LLVM_LIBS_LIST "-lLLVMSupport")    # Remove element

macro(add_clang_plugin name)
    set (srcs ${ARGN})

    include_directories(${CLANG_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
    link_directories("${LLVM_INSTALL_PREFIX}/lib" ${LLVM_LIBRARY_DIRS})

    add_library( ${name} SHARED ${srcs} )

    if (SYMBOL_FILE)
        set_target_properties( ${name} PROPERTIES LINK_FlAGS
            "-exported_symbols_list ${SYMBOL_FILE}")
    endif()

    foreach (clang_lib ${CLANG_LIBS})
        target_link_libraries( ${name} ${clang_lib} )
    endforeach()

    foreach (llvm_lib ${LLVM_LIBS_LIST})
        target_link_libraries( ${name} ${llvm_lib} )
    endforeach()

    foreach (user_lib ${USER_LIBS})
        target_link_libraries( ${name} ${user_lib} )
    endforeach()

endmacro(add_clang_plugin)


set(SYMBOL_FILE Lazy.exports)

set(SRC_FILES
    checkbase.cpp
    checkmanager.cpp
    checks/autounexpectedqstringbuilder.cpp
    checks/assertwithsideeffects.cpp
    checks/container-anti-pattern.cpp
    checks/container-inside-loop.cpp
    checks/copyable-polymorphic.cpp
    checks/detachingbase.cpp
    checks/detachingmember.cpp
    checks/detachingtemporary.cpp
    checks/dynamic_cast.cpp
    checks/foreach.cpp
    checks/globalconstcharpointer.cpp
    checks/isempty-vs-count.cpp
    checks/inefficientqlistbase.cpp
    checks/inefficientqlist.cpp
    checks/inefficient-qlist-soft.cpp
    checks/implicitcasts.cpp
    checks/lambda-in-connect.cpp
    checks/qdatetimeutc.cpp
    checks/qgetenv.cpp
    checks/qvariant-template-instantiation.cpp
    checks/function-args-by-ref.cpp
    checks/function-args-by-value.cpp
    checks/qdeleteall.cpp
    checks/qfileinfo-exists.cpp
    checks/qmap-with-pointer-key.cpp
    checks/qstringarg.cpp
    checks/qstringref.cpp
    checks/qstring-allocations.cpp
    checks/qstring-insensitive-allocation.cpp
    checks/qt4-qstring-from-array.cpp
    checks/missing-type-info.cpp
    checks/missing-qobject-macro.cpp
    checks/nonpodstatic.cpp
    checks/range-loop.cpp
    checks/requiredresults.cpp
    checks/reservecandidates.cpp
    checks/ruleofbase.cpp
    checks/ruleoftwosoft.cpp
    checks/ruleofthree.cpp
    checks/temporaryiterator.cpp
    checks/unused-non-trivial-variable.cpp
    checks/virtualcallsfromctor.cpp
    checks/writingtotemporary.cpp
    checks/wrong-qglobalstatic.cpp
    Clazy.cpp
    ContextUtils.cpp
    FixItUtils.cpp
    HierarchyUtils.cpp
    LoopUtils.cpp
    MacroUtils.cpp
    TemplateUtils.cpp
    StringUtils.cpp
    SuppressionManager.cpp
    TypeUtils.cpp
    QtUtils.cpp
    Utils.cpp)

if (HAS_STD_REGEX)
    set(SRC_FILES ${SRC_FILES} checks/oldstyleconnect.cpp)
else()
    add_definitions(-DNO_STD_REGEX)
    message("old-style-connect check is disabled due to missing std::regex support")
    message("Suppressions are disabled due to missing std::regex support")
endif()

add_clang_plugin(ClangLazy ${SRC_FILES})

set_target_properties(ClangLazy PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX "")

include("GNUInstallDirs")
install(TARGETS ClangLazy     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                              LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                              ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(NOT WIN32)
    install(FILES ${CMAKE_SOURCE_DIR}/clazy DESTINATION bin PERMISSIONS OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
else()
    if(MSVC)
        set(CLANG_COMPILER "clang-cl")
    else()
        set(CLANG_COMPILER "clang++")
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/clazy.bat.cmake ${CMAKE_BINARY_DIR}/clazy.bat)
    install(FILES ${CMAKE_BINARY_DIR}/clazy.bat DESTINATION bin PERMISSIONS OWNER_WRITE OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
endif()
