#!/usr/bin/env python3
"""Apply only selected fixits from an exported clang fixit YAML file."""

import argparse
import os
import subprocess
import sys
import tempfile
import time

try:
    import yaml
except ImportError:
    print('ERROR: PyYAML is required. Install it with `pip install pyyaml`.', file=sys.stderr)
    sys.exit(1)


def load_yaml(path):
    with open(path, 'r', encoding='utf-8') as fh:
        return yaml.safe_load(fh)


def dump_yaml(data, path):
    with open(path, 'w', encoding='utf-8') as fh:
        yaml.safe_dump(data, fh, sort_keys=False)


def make_hashable(value):
    if isinstance(value, dict):
        return tuple(sorted((key, make_hashable(val)) for key, val in value.items()))
    if isinstance(value, list):
        return tuple(make_hashable(item) for item in value)
    if isinstance(value, tuple):
        return tuple(make_hashable(item) for item in value)
    return value


def dedupe_diagnostics(items):
    seen = set()
    unique = []
    for item in items:
        if not isinstance(item, dict):
            continue
        key = make_hashable(item)
        if key in seen:
            continue
        seen.add(key)
        unique.append(item)
    return unique


def run_clang_apply_replacements(yaml_path):
    command = os.getenv('CLAZY_CLANG_APPLY_REPLACEMENTS', 'clang-apply-replacements')
    result = subprocess.run([command, yaml_path])
    print(result)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description='Apply only selected fixits from a clang fixit YAML export.')
    parser.add_argument('yaml_file', help='Exported fixit YAML file')
    parser.add_argument('diagnostics', nargs='+', help='Diagnostic names to apply')
    args = parser.parse_args()

    data = load_yaml(args.yaml_file)
    if not isinstance(data, dict) or 'Diagnostics' not in data:
        print('ERROR: Expected a YAML document with a top-level Diagnostics list.', file=sys.stderr)
        return 2

    diagnostics = data.get('Diagnostics')
    if not isinstance(diagnostics, list):
        print('ERROR: Diagnostics section is not a list.', file=sys.stderr)
        return 2

    names = set(args.diagnostics)
    filtered = [item for item in diagnostics if isinstance(item, dict) and item.get('DiagnosticName') in names]
    # filtered = [item for item in filtered if item and item.get("DiagnosticMessage").get("FilePath").endswith("mymoneystatementreader.cpp")]

    filtered = dedupe_diagnostics(filtered)
    print(filtered)

    if not filtered:
        print('No matching diagnostics found for:', ', '.join(args.diagnostics), file=sys.stderr)
        return 1

    data['Diagnostics'] = filtered

    print("Dumping filtered yaml")
    yaml_name = os.path.basename(args.yaml_file) or 'fixes.yaml'
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = os.path.join(temp_dir, yaml_name)
        dump_yaml(data, temp_path)
        print(temp_path)
        time.sleep(30)

        return_code = run_clang_apply_replacements(temp_dir)
        if return_code != 0:
            print('clang-apply-replacements failed with exit code', return_code, file=sys.stderr)
        return return_code


if __name__ == '__main__':
    sys.exit(main())
