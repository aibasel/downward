#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
Run some syntax checks. Return 0 if all tests pass and 1 otherwise.
"""

from __future__ import print_function

import glob
import os
import re
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")


def check_translator_style():
    output = subprocess.check_output([
        "./reindent.py", "--dryrun", "--recurse", "--verbose",
        os.path.join(SRC_DIR, "translate")], cwd=DIR).decode("utf-8")
    ok = True
    for line in output.splitlines():
        match = re.match("^checking (.+) ... changed.$", line)
        if match:
            ok = False
            print('Wrong format detected in %s. '
                  'Please run "./reindent.py -r ../src/translate"' %
                  match.group(1))
    return ok


def _run_pyflakes(path):
    python_files = []
    for root, dirs, files in os.walk(path):
        python_files.extend([
            os.path.join(root, f) for f in files
            if f.endswith(".py") and f != "__init__.py"])
    try:
        return subprocess.check_call(["pyflakes"] + python_files) == 0
    except OSError as err:
        if err.errno == 2:
            print(
                "Python style checks need pyflakes. Please install it "
                "with \"sudo apt-get install pyflakes\".")
        else:
            raise

def check_translator_pyflakes():
    return _run_pyflakes(os.path.join(SRC_DIR, "translate"))

def check_driver_pyflakes():
    return _run_pyflakes(os.path.join(REPO, "driver"))


def check_include_guard_convention():
    return subprocess.call("./check-include-guard-convention.py", cwd=DIR) == 0


def check_cc_files():
    """
    Currently, we only check that there is no "std::" in .cc files.
    """
    search_dir = os.path.join(SRC_DIR, "search")
    cc_files = (
        glob.glob(os.path.join(search_dir, "*.cc")) +
        glob.glob(os.path.join(search_dir, "*", "*.cc")))
    return subprocess.call(["./check-cc-file.py"] + cc_files, cwd=DIR) == 0


def check_cplusplus_style():
    """
    Calling the uncrustify mercurial extension doesn't work for
    bitbucket pipelines (the local .hg/hgrc file is untrusted and
    ignored by mercurial). We therefore find the source files manually
    and call uncrustify directly.
    """
    src_files = []
    for root, dirs, files in os.walk(REPO):
        for ignore_dir in ["builds", "data"]:
            if ignore_dir in dirs:
                dirs.remove(ignore_dir)
        src_files.extend([
            os.path.join(root, file)
            for file in files if file.endswith((".h", ".cc"))])
    config_file = os.path.join(REPO, ".uncrustify.cfg")
    returncode = subprocess.call(
        ["uncrustify", "-q", "-c", config_file, "--check"] + src_files,
        cwd=DIR, stdout=subprocess.PIPE)
    if returncode != 0:
        print(
            'Run "hg uncrustify" to fix the coding style in the above '
            'mentioned files.')
    return returncode == 0


def main():
    tests = [test for test_name, test in sorted(globals().items())
             if test_name.startswith("check_")]
    results = [test() for test in tests]
    if all(results):
        print("All style checks passed")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
