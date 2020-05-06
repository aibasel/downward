#! /usr/bin/env python3

"""
Run some syntax checks. Return 0 if all tests pass and 1 otherwise.

The file bitbucket-pipelines.yml shows how to install the dependencies.
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
            "--extend-ignore", "E128,E129,E131,E261,E266,E301,E302,E305,E306,E402,E501,F401",
            "--exclude", "run-clang-tidy.py,txt2tags.py,.tox",
            "src/translate/", "driver/", "misc/", "*.py"], cwd=REPO)
    except OSError as err:
        if err.errno == errno.ENOENT:
            sys.exit('Error: flake8 not found. Try "tox -e style".')
        else:
            raise
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
    """
    Calling the uncrustify mercurial extension doesn't work for
    bitbucket pipelines (the local .hg/hgrc file is untrusted and
    ignored by mercurial). We therefore find the source files manually
    and call uncrustify directly.
    """
    src_files = utils.get_src_files(
        REPO, (".h", ".cc"), ignore_dirs=["builds", "data", "venv"])
    print("Checking {} files with uncrustify".format(len(src_files)))
    config_file = os.path.join(REPO, ".uncrustify.cfg")
    executable = "uncrustify"
    try:
        returncode = subprocess.call(
            [executable, "-q", "-c", config_file, "--check"] + src_files,
            cwd=DIR, stdout=subprocess.PIPE)
    except OSError as err:
        if err.errno == errno.ENOENT:
            sys.exit("Error: {} not found. Is it on the PATH?".format(executable))
        else:
            raise
    if returncode != 0:
        print(
            'Run "hg uncrustify" to fix the coding style in the above '
            'mentioned files.')
    return returncode == 0


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
