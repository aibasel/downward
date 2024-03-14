#! /usr/bin/env python3

"""
Run syntax checks on Python and C++ files.

Exit with 0 if all tests pass and with 1 otherwise.
"""


import errno
import os
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")

import utils


def check_python_style():
    try:
        subprocess.check_call([
            "flake8",
            # https://flake8.pycqa.org/en/latest/user/error-codes.html
            "--extend-ignore", "E128,E129,E131,E261,E266,E301,E302,E305,E306,E402,E501,E741,F401",
            "--exclude", "run-clang-tidy.py,txt2tags.py,.tox,.venv",
            "src/translate/", "driver/", "misc/",
            "build.py", "build_configs.py", "fast-downward.py"], cwd=REPO)
    except FileNotFoundError:
        sys.exit('Error: flake8 not found. Try "tox -e style".')
    except subprocess.CalledProcessError:
        return False
    else:
        return True


def check_include_guard_convention():
    return subprocess.call("./check-include-guard-convention.py", cwd=DIR) == 0


def check_cc_files():
    """
    Currently, we only check that there is no "std::" in .cc files.
    """
    search_dir = os.path.join(SRC_DIR, "search")
    cc_files = utils.get_src_files(search_dir, (".cc",))
    print("Checking style of {} *.cc files".format(len(cc_files)))
    return subprocess.call(["./check-cc-file.py"] + cc_files, cwd=DIR) == 0


def check_cplusplus_style():
    return subprocess.call(["./run-uncrustify.py"], cwd=DIR) == 0


def main():
    results = []
    for test_name, test in sorted(globals().items()):
        if test_name.startswith("check_"):
            print("Running {}".format(test_name))
            results.append(test())
    if all(results):
        print("All style checks passed")
    else:
        sys.exit("Style checks failed")


if __name__ == "__main__":
    main()
