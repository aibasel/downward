#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
Run some syntax checks. Return 0 if all tests pass and 1 otherwise.

The file bitbucket-pipelines.yml shows how to install the dependencies.
"""

from __future__ import print_function

import errno
import glob
import json
import os
import pipes
import re
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")


def _get_src_files(path, extensions, ignore_dirs=None):
    ignore_dirs = ignore_dirs or []
    src_files = []
    for root, dirs, files in os.walk(path):
        for ignore_dir in ignore_dirs:
            if ignore_dir in dirs:
                dirs.remove(ignore_dir)
        src_files.extend([
            os.path.join(root, file)
            for file in files if file.endswith(extensions)])
    return src_files


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
    python_files = _get_src_files(path, (".py",))
    python_files = [f for f in python_files if not f.endswith("__init__.py")]
    print("Checking {} files with pyflakes".format(len(python_files)))
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
    cc_files = _get_src_files(search_dir, (".cc",))
    print("Checking style of {} *.cc files".format(len(cc_files)))
    return subprocess.call(["./check-cc-file.py"] + cc_files, cwd=DIR) == 0


def check_cplusplus_style():
    """
    Calling the uncrustify mercurial extension doesn't work for
    bitbucket pipelines (the local .hg/hgrc file is untrusted and
    ignored by mercurial). We therefore find the source files manually
    and call uncrustify directly.
    """
    src_files = _get_src_files(REPO, (".h", ".cc"), ignore_dirs=["builds", "data"])
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


def check_search_code_with_clang_tidy():
    # clang-tidy needs the CMake files.
    build_dir = os.path.join(REPO, "builds", "clang-tidy")
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    with open(os.devnull, 'w') as devnull:
        subprocess.check_call(["cmake", "../../src"], cwd=build_dir, stdout=devnull)

    # Create custom compilation database file. CMake outputs part of this information
    # when passing -DCMAKE_EXPORT_COMPILE_COMMANDS=ON, but the resulting file
    # contains no header files.
    search_dir = os.path.join(REPO, "src/search")
    src_files = _get_src_files(search_dir, (".h", ".cc"))
    compile_commands = [{
        "directory": os.path.join(build_dir, "search"),
        "command": "g++ -I{search_dir}/ext -std=c++11 -c {src_file}".format(**locals()),
        "file": src_file}
        for src_file in src_files
    ]
    with open(os.path.join(build_dir, "compile_commands.json"), "w") as f:
        json.dump(compile_commands, f, indent=2)

    # See https://clang.llvm.org/extra/clang-tidy/checks/list.html for
    # an explanation of the checks. We comment out inactive checks of some
    # categories instead of deleting them to see which additional checks
    # we could activate.
    checks = [
        # Enable with CheckTriviallyCopyableMove=0 when we require
        # clang-tidy >= 6.0 (see issue856).
        #"misc-move-const-arg",
        "misc-move-constructor-init",
        "misc-use-after-move",

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
        # Disabled since we prefer a clean interface over consistent names.
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
    print("Running clang-tidy: " + " ".join(pipes.quote(x) for x in cmd))
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
