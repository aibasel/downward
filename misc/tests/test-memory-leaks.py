#! /usr/bin/env python

from __future__ import print_function

import errno
import os
import pipes
import re
import subprocess
import sys

import configs

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
BENCHMARKS_DIR = os.path.join(REPO, "misc", "tests", "benchmarks")
FAST_DOWNWARD = os.path.join(REPO, "fast-downward.py")
BUILD_DIR = os.path.join(REPO, "builds", "release")
DOWNWARD_BIN = os.path.join(BUILD_DIR, "bin", "downward")
SAS_FILE = os.path.join(REPO, "test.sas")
PLAN_FILE = os.path.join(REPO, "test.plan")
VALGRIND_GCC5_SUPPRESSION_FILE = os.path.join(
    REPO, "misc", "tests", "valgrind", "gcc5.supp")
VALGRIND_ERROR_EXITCODE = 99

TASKS = [os.path.join(BENCHMARKS_DIR, path) for path in [
    "miconic/s1-0.pddl",
]]

CONFIGS = {}
CONFIGS.update(configs.default_configs_optimal(core=True, extended=True))
CONFIGS.update(configs.default_configs_satisficing(core=True, extended=True))


def escape_list(l):
    return " ".join(pipes.quote(x) for x in l)


def get_compiler_and_version():
    output = subprocess.check_output(["cmake", "-LA", "-N", "../../src/"], cwd=BUILD_DIR)
    compiler = re.search("^DOWNWARD_CXX_COMPILER_ID:STRING=(.+)$", output, re.M).group(1)
    version = re.search("^DOWNWARD_CXX_COMPILER_VERSION:STRING=(.+)$", output, re.M).group(1)
    return compiler, version


def run_plan_script(task, config, suppression_files):
    assert "--alias" not in config, config
    cmd = [
        "valgrind",
        "--leak-check=full",
        "--error-exitcode={}".format(VALGRIND_ERROR_EXITCODE),
        "--show-leak-kinds=all",
        "--errors-for-leak-kinds=all",
        "--track-origins=yes"]
    for suppression_file in suppression_files:
        cmd.append("--suppressions={}".format(suppression_file))
    cmd.extend([DOWNWARD_BIN] + config + ["--internal-plan-file", PLAN_FILE])
    print("\nRun: {}".format(escape_list(cmd)))
    sys.stdout.flush()
    try:
        subprocess.check_call(cmd, stdin=open(SAS_FILE), cwd=REPO)
    except OSError as err:
        if err.errno == errno.ENOENT:
            sys.exit(
                "Could not find valgrind. Please install it "
                "with \"sudo apt install valgrind\".")
    except subprocess.CalledProcessError as err:
        # Valgrind exits with
        #   - the exit code of the binary if it does not find leaks
        #   - VALGRIND_ERROR_EXITCODE if it finds leaks
        #   - 1 in case of usage errors
        # Fortunately, we only use exit code 1 for portfolios.
        if err.returncode == 1:
            sys.exit("\nError: failed to run valgrind")
        elif err.returncode == VALGRIND_ERROR_EXITCODE:
            sys.exit(
                "\nError: {config} leaks memory for {task}".format(**locals()))


def translate(task):
    subprocess.check_call([
        sys.executable, FAST_DOWNWARD, "--sas-file", SAS_FILE, "--translate", task], cwd=REPO)


def cleanup():
    os.remove(SAS_FILE)
    os.remove(PLAN_FILE)


def main():
    subprocess.check_call(["./build.py"], cwd=REPO)
    compiler, compiler_version = get_compiler_and_version()
    print("Compiler:", compiler, compiler_version)
    suppression_files = []
    if compiler == "GNU" and compiler_version.split(".")[0] == "5":
        print("Using leak suppression file for GCC 5 "
              "(see http://issues.fast-downward.org/issue703).")
        suppression_files.append(VALGRIND_GCC5_SUPPRESSION_FILE)
    for task in TASKS:
        translate(task)
        for config in CONFIGS.values():
            run_plan_script(task, config, suppression_files)
    cleanup()
    print("No memory leaks found.")


main()
