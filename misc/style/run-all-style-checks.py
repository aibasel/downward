#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
Run some syntax checks. Return 0 if all tests pass and 1 otherwise.

The file bitbucket-pipelines.yml shows how to install the dependencies.
"""

from __future__ import print_function

import glob
import os
import pipes
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


def check_search_code_with_clang_tidy():
    # clang-tidy needs the CMake files.
    build_dir = os.path.join(REPO, "builds", "clang-tidy")
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    with open(os.devnull, 'w') as devnull:
        subprocess.check_call(
            ["cmake", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "../../src"],
            cwd=build_dir, stdout=devnull)
    # See https://clang.llvm.org/extra/clang-tidy/checks/list.html for
    # an explanation of the checks. We comment out inactive checks of some
    # categories instead of deleting them to see which additional checks
    # we could activate.
    checks = [
        "performance-for-range-copy",
        "performance-implicit-cast-in-loop",
        "performance-inefficient-vector-operation",

        "readability-avoid-const-params-in-decls",
        #"readability-braces-around-statements",
        "readability-container-size-empty",
        "readability-delete-null-pointer",
        "readability-deleted-default",
        #"readability-else-after-return",
        #"readability-function-size",
        #"readability-identifier-naming",
        #"readability-implicit-bool-cast",
        #"readability-inconsistent-declaration-parameter-name",
        "readability-misleading-indentation",
        "readability-misplaced-array-index",
        #"readability-named-parameter",
        #"readability-non-const-parameter",
        "readability-redundant-control-flow",
        "readability-redundant-declaration",
        "readability-redundant-function-ptr-dereference",
        "readability-redundant-member-init",
        "readability-redundant-smartptr-get",
        "readability-redundant-string-cstr",
        "readability-redundant-string-init",
        "readability-simplify-boolean-expr",
        "readability-static-definition-in-anonymous-namespace",
        "readability-uniqueptr-delete-release",
        ]
    cmd = [
        "./run-clang-tidy.py",
        "-quiet",
        "-p", build_dir,
        "-clang-tidy-binary=clang-tidy-5.0",
        # Include all non-system headers (.*) except the ones from search/ext/.
        "-header-filter=.*,-tree.hh,-tree_util.hh",
        "-checks=-*," + ",".join(checks)]
    print("Running clang-tidy (enabled checks: {})".format(", ".join(checks)))
    print()
    try:
        output = subprocess.check_output(cmd, cwd=DIR, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
        print("Failed to run clang-tidy-5.0. Is it on the PATH?")
        return False
    errors = re.findall(r"^(.*:\d+:\d+: (?:warning|error): .*)$", output, flags=re.M)
    for error in errors:
        print(error)
    if errors:
        fix_cmd = cmd + [
            "-clang-apply-replacements-binary=clang-apply-replacements-5.0", "-fix"]
        print()
        print("You may be able to fix these issues with the following command: " +
            " ".join(pipes.quote(x) for x in fix_cmd))
    return not errors


def main():
    tests = [test for test_name, test in sorted(globals().items())
             if test_name.startswith("check_")]
    results = [test() for test in tests]
    if all(results):
        print("All style checks passed")
    else:
        sys.exit("Style checks failed")


if __name__ == "__main__":
    main()
