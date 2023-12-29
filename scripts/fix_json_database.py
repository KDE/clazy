#!/usr/bin/env python2


# # SPDX-FileCopyrightText: 2017 Sergio Martins <smartins@kde.org>

# SPDX-License-Identifier: LGPL-2.0-or-later

# This script is useful for fixing the json database, in case there's compiler flags
# you want to remove or add. One such example is when you're required to build your
# project with Apple clang, which will then generate pch errors when running
# clazy.

import json, sys, string

#-------------------------------------------------------------------------------
# Filters. Configure here.

# Removes arguments starting with -include
def filter_no_pch(str):
    return not (str.startswith("-include") or str.startswith(".pch/"))

def remove_pch(c_splitted):
    return filter(filter_no_pch, c_splitted)

def fix_command(c):
    c_splitted = c.split()
    c_splitted = remove_pch(c_splitted)
    return string.join(c_splitted)


def fix_arguments(args):
    args = remove_pch(args)
    return args

#-------------------------------------------------------------------------------
# Main:

f = open(sys.argv[1], 'r')
contents = f.read()
f.close()

decoded = json.loads(contents)
new_decoded = []
for cmd in decoded:
    if 'file' in cmd:
        if cmd['file'].endswith('.moc') or cmd['file'].endswith('.rcc') or cmd['file'].endswith('_moc.cpp'):
            continue

    if 'command' in cmd or 'arguments' in cmd:
        if 'command' in cmd:
            cmd['command'] = fix_command(cmd['command'])
        if 'arguments' in cmd:
            cmd['arguments'] = fix_arguments(cmd['arguments'])

        new_decoded.append(cmd)

new_contents = json.dumps(new_decoded)
print new_contents
