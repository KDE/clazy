#!/usr/bin/env python3

"""
Generates tests/compile_commands.json file. This file is specific to the respective setup and
allows a LSP to provide proper autocompletion for the testfiles.

The generated file should NOT be versioned!
"""

import os
import glob
import json
import shutil
import subprocess
import re

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
CLANG = shutil.which("clang")

qt_headers = subprocess.check_output(["qmake", "-query", "QT_INSTALL_HEADERS"], stderr=subprocess.STDOUT, text=True).strip()

result = subprocess.run(['clang', '-v'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, text=True)
match = re.search(r'Selected .* installation: (.*)', result.stderr)
c_headerpath = match.group(1).strip()

COMMON_FLAGS = f"-Wno-unused-value -Qunused-arguments -std=c++17 -isystem {c_headerpath} -isystem {qt_headers}"

tests_dir = os.path.join(ROOT, "tests")
output_path = os.path.join(tests_dir, "compile_commands.json")

pattern1 = os.path.join(tests_dir, "**", "*.cpp")
pattern2 = os.path.join(tests_dir, "*.cpp")

files = glob.glob(pattern1, recursive=True) + glob.glob(pattern2)
files = [f for f in files if os.path.isfile(f)]

entries = []

for f in files:
    entry = {
        "directory": ROOT,
        "command": f"{CLANG} {COMMON_FLAGS} -c {f}",
        "file": f
    }
    entries.append(entry)

with open(output_path, "w") as outfile:
    json.dump(entries, outfile, indent=2)
