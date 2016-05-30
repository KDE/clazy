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

    target_link_libraries( ${name} clazylib )

endmacro(add_clang_plugin)

# clazylib version
set(clazylib_VERSION_MAJOR 0)
set(clazylib_VERSION_MINOR 1)
# Enable the full x.y.z version only for release versions
set(clazylib_VERSION_PATCH 0)
set(clazylib_VERSION ${clazylib_VERSION_MAJOR}.${clazylib_VERSION_MINOR})

include_directories(${CMAKE_BINARY_DIR})
set(CLAZY_LIB_SRC
    checkbase.cpp
    checkmanager.cpp
    SuppressionManager.cpp
    ContextUtils.cpp
    FixItUtils.cpp
    HierarchyUtils.cpp
    LoopUtils.cpp
    MacroUtils.cpp
    QtUtils.cpp
    StringUtils.cpp
    TemplateUtils.cpp
    TypeUtils.cpp
    Utils.cpp)

add_library(clazylib SHARED
    ${CLAZY_LIB_SRC}
    )

set_target_properties(clazylib PROPERTIES VERSION ${clazylib_VERSION} SOVERSION ${clazylib_VERSION_MAJOR})
include("GNUInstallDirs")

install(TARGETS clazylib EXPORT LibClaryExport
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

set(SYMBOL_FILE Lazy.exports)

set(SRC_FILES
    #level0
    checks/level0/container-anti-pattern.cpp
    checks/level0/lambda-in-connect.cpp
    checks/level0/qdatetimeutc.cpp
    checks/level0/qfileinfo-exists.cpp
    checks/level0/qgetenv.cpp
    checks/level0/qmap-with-pointer-key.cpp
    checks/level0/qstringarg.cpp
    checks/level0/qstring-insensitive-allocation.cpp
    checks/level0/qstringref.cpp
    checks/level0/qvariant-template-instantiation.cpp
    checks/level0/temporaryiterator.cpp
    checks/level0/unused-non-trivial-variable.cpp
    checks/level0/writingtotemporary.cpp
    checks/level0/wrong-qglobalstatic.cpp

    #level1
    checks/level1/autounexpectedqstringbuilder.cpp
    checks/level1/detachingtemporary.cpp
    checks/level1/foreach.cpp
    checks/level1/inefficient-qlist-soft.cpp
    checks/level1/missing-qobject-macro.cpp
    checks/level1/nonpodstatic.cpp
    checks/level1/qdeleteall.cpp
    checks/level1/qstring-left.cpp
    checks/level1/range-loop.cpp

    #level2
    checks/level2/container-inside-loop.cpp
    checks/level2/function-args-by-ref.cpp
    checks/level2/function-args-by-value.cpp
    checks/level2/globalconstcharpointer.cpp
    checks/level2/implicitcasts.cpp
    checks/level2/missing-type-info.cpp
    #checks/level2/oldstyleconnect.cpp) requires std::regex, see below
    checks/level2/qstring-allocations.cpp
    checks/level2/reservecandidates.cpp
    checks/level2/ruleofthree.cpp
    checks/level2/ruleoftwosoft.cpp
    checks/level2/virtualcallsfromctor.cpp

    #level3
    checks/level3/assertwithsideeffects.cpp
    checks/level3/copyable-polymorphic.cpp
    checks/level3/detachingmember.cpp
    checks/level3/dynamic_cast.cpp

    #hiddenlevel
    checks/hiddenlevel/inefficientqlist.cpp
    checks/hiddenlevel/isempty-vs-count.cpp
    checks/hiddenlevel/qt4-qstring-from-array.cpp

    #support classes
    checks/detachingbase.cpp
    checks/inefficientqlistbase.cpp
    checks/requiredresults.cpp
    checks/ruleofbase.cpp

    #top-level worker
    Clazy.cpp
)

if (HAS_STD_REGEX)
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
    PREFIX "")

install(TARGETS ClangLazy     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                              LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                              ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

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

file(GLOB README_LEVEL0_FILES checks/level0/README-*)
file(GLOB README_LEVEL1_FILES checks/level1/README-*)
file(GLOB README_LEVEL2_FILES checks/level2/README-*)
file(GLOB README_LEVEL3_FILES checks/level3/README-*)
file(GLOB README_HIDDENLEVEL_FILES checks/hiddenlevel/README-*)
install(FILES ${README_LEVEL0_FILES} DESTINATION ${SHARE_INSTALL_DIR}/clazy/doc/level0)
install(FILES ${README_LEVEL1_FILES} DESTINATION ${SHARE_INSTALL_DIR}/clazy/doc/level1)
install(FILES ${README_LEVEL2_FILES} DESTINATION ${SHARE_INSTALL_DIR}/clazy/doc/level2)
install(FILES ${README_LEVEL3_FILES} DESTINATION ${SHARE_INSTALL_DIR}/clazy/doc/level3)
install(FILES ${README_HIDDENLEVEL_FILES} DESTINATION ${SHARE_INSTALL_DIR}/clazy/doc/hiddenlevel)

set(CLAZY_LIB_INCLUDES
    checkbase.h
    checkmanager.h
    SuppressionManager.h
    ContextUtils.h
    FixItUtils.h
    HierarchyUtils.h
    LoopUtils.h
    MacroUtils.h
    QtUtils.h
    StringUtils.h
    TemplateUtils.h
    TypeUtils.h
    Utils.h
    clazy_stl.h
    clazylib_export.h
    )

install(FILES ${CLAZY_LIB_INCLUDES} DESTINATION include/clazy )
