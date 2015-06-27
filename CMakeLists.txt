cmake_minimum_required (VERSION 2.6)
project (ClangMoreWarningsPlugin)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

add_definitions (-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions (-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

set (LLVM_INSTALL_PREFIX /data/prefix/)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fno-common -Woverloaded-virtual -Wcast-qual -fno-strict-aliasing -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings -fno-exceptions -fno-rtti")
set (CMAKE_MODULE_LINKER_FLAGS "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress")

set (LLVM_LIBS
    #LLVMJIT
    #LLVMX86CodeGen
    #LLVMX86AsmParser
    #LLVMX86Disassembler
    #LLVMExecutionEngine
    #LLVMAsmPrinter
    #LLVMSelectionDAG
    #LLVMX86AsmPrinter
    #LLVMX86Info
    #LLVMMCParser
    #LLVMCodeGen
    #LLVMX86Utils
    #LLVMScalarOpts
    #LLVMInstCombine
    #LLVMTransformUtils
    #LLVMipa
    #LLVMAnalysis
    #LLVMTarget
    #LLVMCore
    #LLVMMC
    #LLVMSupport
)

macro(add_clang_plugin name)
    set (srcs ${ARGN})

    include_directories("${LLVM_INSTALL_PREFIX}/include" ${CMAKE_SOURCE_DIR})

    link_directories("${LLVM_INSTALL_PREFIX}/lib")

    add_library( ${name} SHARED ${srcs} )

    if (SYMBOL_FILE)
        set_target_properties( ${name} PROPERTIES LINK_FlAGS
            "-exported_symbols_list ${SYMBOL_FILE}")
    endif()

    foreach (clang_lib ${CLANG_LIBS})
        target_link_libraries( ${name} ${clang_lib} )
    endforeach()

    foreach (llvm_lib ${LLVM_LIBS})
        target_link_libraries( ${name} ${llvm_lib} )
    endforeach()

    foreach (user_lib ${USER_LIBS})
        target_link_libraries( ${name} ${user_lib} )
    endforeach()

endmacro(add_clang_plugin)


set(SYMBOL_FILE MoreWarningsPlugin.exports)

set (CLANG_LIBS
    #clang
    #clangFrontend
    #clangAST
    #clangAnalysis
    #clangBasic
    #clangCodeGen
    #clangDriver
    #clangFrontendTool
    #clangIndex
    #clangLex
    #clangParse
    #clangRewrite
    #clangSema
    #clangSerialization
    #clangStaticAnalyzerCheckers
    #clangStaticAnalyzerCore
    #clangStaticAnalyzerFrontend
)

set (USER_LIBS
    #pthread
)

add_clang_plugin(ClangMoreWarningsPlugin
                 checkbase.cpp
                 checks/assertwithsideeffects.cpp
                 checks/detachingtemporaries.cpp
                 checks/duplicateexpensivestatement.cpp
                 checks/dynamic_cast.cpp
                 checks/foreachdetachments.cpp
                 checks/inefficientqlist.cpp
                 checks/qlistint.cpp
                 checks/qmapkey.cpp
                 checks/movablecontainers.cpp
                 checks/nonpodstatic.cpp
                 checks/nrvoenabler.cpp
                 checks/requiredresults.cpp
                 checks/reserveadvisor.cpp
                 checks/variantsanitizer.cpp
                 checks/virtualcallsfromctor.cpp
                 MoreWarningsPlugin.cpp
                 Utils.cpp
                 )

set_target_properties(ClangMoreWarningsPlugin PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX "")

install(TARGETS ClangMoreWarningsPlugin DESTINATION lib)
