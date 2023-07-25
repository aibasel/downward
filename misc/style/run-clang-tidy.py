#! /usr/bin/env python3

import json
import os
import pipes
import re
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")

import utils


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
    src_files = utils.get_src_files(search_dir, (".h", ".cc"))
    compile_commands = [{
        "directory": os.path.join(build_dir, "search"),
        "command": "g++ -std=c++20 -c {}".format(src_file),
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
        # "misc-move-const-arg",
        "misc-move-constructor-init",
        "misc-use-after-move",

        "performance-for-range-copy",
        "performance-implicit-cast-in-loop",
        "performance-inefficient-vector-operation",

        "readability-avoid-const-params-in-decls",
        # "readability-braces-around-statements",
        "readability-container-size-empty",
        "readability-delete-null-pointer",
        "readability-deleted-default",
        # "readability-else-after-return",
        # "readability-function-size",
        # "readability-identifier-naming",
        # "readability-implicit-bool-cast",
        # Disabled since we prefer a clean interface over consistent names.
        # "readability-inconsistent-declaration-parameter-name",
        "readability-misleading-indentation",
        "readability-misplaced-array-index",
        # "readability-named-parameter",
        # "readability-non-const-parameter",
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
        "run-clang-tidy-12",
        "-quiet",
        "-p", build_dir,
        "-clang-tidy-binary=clang-tidy-12",
        "-checks=-*," + ",".join(checks)]
    print("Running clang-tidy: " + " ".join(pipes.quote(x) for x in cmd))
    print()
    try:
        output = subprocess.check_output(cmd, cwd=DIR, stderr=subprocess.STDOUT).decode("utf-8")
    except subprocess.CalledProcessError as err:
        print("Failed to run clang-tidy-12. Is it on the PATH?")
        print("Output:", err.stdout)
        return False
    errors = re.findall(r"^(.*:\d+:\d+: .*(?:warning|error): .*)$", output, flags=re.M)
    for error in errors:
        print(error)
    if errors:
        fix_cmd = cmd + [
            "-clang-apply-replacements-binary=clang-apply-replacements-12", "-fix"]
        print()
        print("You may be able to fix these issues with the following command: " +
            " ".join(pipes.quote(x) for x in fix_cmd))
        sys.exit(1)


check_search_code_with_clang_tidy()
