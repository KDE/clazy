enable_testing()

if ($ENV{CI_JOB_NAME_SLUG} MATCHES "qt5")
    set(TEST_VERSION_OPTION "--qt-versions=5")
elseif($ENV{CI_JOB_NAME_SLUG} MATCHES "qt6")
    set(TEST_VERSION_OPTION "--qt-versions=6")
endif()
if (NOT CLAZY_BUILD_CLANG_TIDY)
    set(TEST_OPTIONS "--no-clang-tidy")
endif()

macro(add_clazy_test name)
  add_test(NAME ${name} COMMAND python3 run_tests.py ${name} --verbose ${TEST_VERSION_OPTION} ${TEST_OPTIONS} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/)
  if (CLANG_EXECUTABLE_PATH)
    set(CLANG_CXX_TEST "CLANGXX=${CLANG_EXECUTABLE_PATH};")
  endif()
  if (CLAZY_BUILD_CLANG_TIDY)
    set(CLANGTIDYPLUGIN_CXX "CLANGTIDYPLUGIN_CXX=$<TARGET_FILE:ClazyClangTidy>;")
  endif()
  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "${CLANG_CXX_TEST}${CLANGTIDYPLUGIN_CXX}CLAZYPLUGIN_CXX=$<TARGET_FILE:ClazyPlugin>;CLAZYSTANDALONE_CXX=$<TARGET_FILE:clazy-standalone>"
  )
endmacro()

include(ClazyTests.generated.cmake)
