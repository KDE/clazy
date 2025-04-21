find_program(GIT_EXECUTABLE git${CMAKE_EXECUTABLE_SUFFIX})
if (GIT_EXECUTABLE)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty OUTPUT_VARIABLE CLAZY_GIT_VERSION ERROR_VARIABLE GIT_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    if (NOT GIT_ERROR)
        if(NOT CLAZY_GIT_VERSION STREQUAL CLAZY_PRINT_VERSION)
            set(CLAZY_PRINT_VERSION "${CLAZY_PRINT_VERSION} (${CLAZY_GIT_VERSION})")
        endif()
    else()
        message(WARNING "Error obtaining version from git")
    endif()
    message("CLAZY_PRINT_VERSION: ${CLAZY_PRINT_VERSION}")
endif()
