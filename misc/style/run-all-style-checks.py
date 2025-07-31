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

checks = []

def check_python_style():
    try:
        subprocess.check_call([
            "flake8",
            # https://flake8.pycqa.org/en/latest/user/error-codes.html
            "--extend-ignore",
            "E128,E129,E131,E261,E266,E301,E302,E305,E306,E402,E501,E741,F401",
            "--exclude", "run-clang-tidy.py,txt2tags.py,.tox,.venv",
            "src/translate/", "driver/", "misc/",
            "build.py", "build_configs.py", "fast-downward.py"], cwd=REPO)
    except FileNotFoundError:
        sys.exit('Error: flake8 not found. Try "tox -e style".')
    except subprocess.CalledProcessError:
        return False
    else:
        return True

checks.append(check_python_style)


def check_include_guard_convention():
    return subprocess.call("./check-include-guard-convention.py", cwd=DIR) == 0

checks.append(check_include_guard_convention)


def check_cc_files():
    """
    Currently, we only check that there is no "std::" in .cc files.
    """
    search_dir = os.path.join(SRC_DIR, "search")
    cc_files = utils.get_src_files(search_dir, (".cc",))
    print("Checking style of {} *.cc files".format(len(cc_files)))
    return subprocess.call(["./check-cc-file.py"] + cc_files, cwd=DIR) == 0

checks.append(check_cc_files)


def check_cplusplus_style():
    return subprocess.call(["./run-clang-format.py"], cwd=DIR) == 0

checks.append(check_cplusplus_style)


def main():
    results = []
    for check in checks:
        print(f"Running {check.__name__}")
        results.append(check())
    if all(results):
        print("All style checks passed")
    else:
        sys.exit("Style checks failed")


if __name__ == "__main__":
    main()
