import io
import os
import subprocess


def compare_files(expects_failure, expected_file, result_file, message, verbose: bool):
    success = files_are_equal(expected_file, result_file)

    if expects_failure:
        if success:
            print("[XOK]   " + message)
            return False
        else:
            print("[XFAIL] " + message)
            print_differences(expected_file, result_file, verbose)
            return True
    else:
        if success:
            print("[OK]   " + message)
            return True
        else:
            print("[FAIL] " + message)
            print_differences(expected_file, result_file, verbose)
            return False



def run_command(cmd, output_file="", test_env=os.environ, cwd=None, verbose = False, ignore_verbose_command=False, qt_namespaced=False, qt_replace_namespace=True):
    lines, success = get_command_output(cmd, test_env=test_env, verbose=verbose, cwd=cwd, ignore_verbose=ignore_verbose_command)
    # Hack for Windows, we have std::_Vector_base in the expected data
    lines = lines.replace("std::_Container_base0", "std::_Vector_base")
    lines = lines.replace("std::__1::__vector_base_common",
                          "std::_Vector_base")  # Hack for macOS
    lines = lines.replace("std::_Vector_alloc", "std::_Vector_base")

    # clang-tidy prints the tags slightly different
    if cmd.startswith("clang-tidy"):
        lines = lines.replace("[clazy", "[-Wclazy")
    if qt_namespaced and qt_replace_namespace:
        lines = lines.replace("MyQt::", "")

    if not success and not output_file:
        print(lines)
        return False

    if verbose:
        print("Running: " + cmd)
        print("output_file=" + output_file)

    lines = lines.replace('\r\n', '\n')
    if lines.endswith('\n\n'):
        lines = lines[:-1]  # remove *only* one extra newline, all testfiles have a newline based on unix convention
    if output_file:
        f = io.open(output_file, 'w', encoding='utf8')
        f.writelines(lines)
        f.close()
    elif len(lines) > 0:
        print(lines)

    return success

def get_command_output(cmd: str, verbose: bool, test_env=os.environ, cwd=None, ignore_verbose=False):
    success = True

    try:
        if verbose and not ignore_verbose:
            print(cmd)

        # Polish up the env to fix "TypeError: environment can only contain strings" exception
        str_env = {}
        for key in test_env.keys():
            str_env[str(key)] = str(test_env[key])

        output = subprocess.check_output(
            cmd, stderr=subprocess.STDOUT, shell=True, env=str_env, cwd=cwd)
    except subprocess.CalledProcessError as e:
        output = e.output
        success = False

    if type(output) is bytes:
        output = output.decode('utf-8')

    return output, success


def files_are_equal(file1, file2):
    try:
        f = io.open(file1, 'r', encoding='utf-8')
        lines1 = f.readlines()
        f.close()

        f = io.open(file2, 'r', encoding='utf-8')
        lines2 = f.readlines()
        f.close()

        return lines1 == lines2
    except Exception as ex:
        print("Error comparing files:" + str(ex))
        return False

def print_differences(file1, file2, verbose):
    # Returns true if the the files are equal
    return run_command("diff -Naur --strip-trailing-cr {} {}".format(file1, file2), verbose=verbose)
