#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
Run some syntax checks. Return 0 if all tests pass and 1 otherwise.
"""

from __future__ import print_function

import os.path
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


def check_include_guard_convention():
    return subprocess.call("./check-include-guard-convention.py", cwd=DIR) == 0


def check_preprocessor_and_search_style():
    output = subprocess.check_output(["hg", "uncrustify", "-X", "re:^src/VAL"])
    if output:
        print('Run "hg uncrustify -m" to fix the style in the following files:')
        print(output.rstrip())
    return not output


def main():
    ok = True
    ok &= check_translator_style()
    ok &= check_include_guard_convention()
    ok &= check_preprocessor_and_search_style()
    if ok:
        print("All style checks passed")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
