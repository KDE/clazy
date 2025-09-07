# SPDX-FileCopyrightText: 2025 Alexander Lohnau <alexander.lohnau@kde.org>
# SPDX-License-Identifier: BSD-2-Clause
set(test_prefix "${CMAKE_BINARY_DIR}/test.prefix.sh")

set(script_content [=[
#!/usr/bin/env bash
export CLAZYPLUGIN_CXX="$<TARGET_FILE:ClazyPlugin>"
export CLAZYSTANDALONE_CXX="$<TARGET_FILE:clazy-standalone>"
]=])

if (CLAZY_BUILD_CLANG_TIDY)
  string(APPEND script_content "export CLANGTIDYPLUGIN_CXX=\"$<TARGET_FILE:ClazyClangTidy>\"\n")
endif()

file(GENERATE
  OUTPUT "${test_prefix}"
  CONTENT "${script_content}"
)

