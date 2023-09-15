import errno
import os
import pipes
import re
import subprocess
import sys

from pathlib import Path
import pytest

import configs

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
BENCHMARKS_DIR = os.path.join(REPO, "misc", "tests", "benchmarks")
FAST_DOWNWARD = os.path.join(REPO, "fast-downward.py")
BUILD_DIR = os.path.join(REPO, "builds", "release")
DOWNWARD_BIN = os.path.join(BUILD_DIR, "bin", "downward")
SAS_FILE = os.path.join(REPO, "test.sas")
PLAN_FILE = os.path.join(REPO, "test.plan")
DLOPEN_SUPPRESSION_FILE = os.path.join(
    REPO, "misc", "tests", "valgrind", "dlopen.supp")
DL_CATCH_ERROR_SUPPRESSION_FILE = os.path.join(
    REPO, "misc", "tests", "valgrind", "dl_catch_error.supp")
VALGRIND_ERROR_EXITCODE = 99

TASK = os.path.join(BENCHMARKS_DIR, "miconic/s1-0.pddl")

CONFIGS = {}
CONFIGS.update(configs.default_configs_optimal(core=True, extended=True))
CONFIGS.update(configs.default_configs_satisficing(core=True, extended=True))

SUPPRESSION_FILES = [
    DLOPEN_SUPPRESSION_FILE,
    DL_CATCH_ERROR_SUPPRESSION_FILE,
]

def escape_list(l):
    return " ".join(pipes.quote(x) for x in l)


def run_plan_script(task, config, custom_suppression_files):
    assert "--alias" not in config, config
    cmd = [
        "valgrind",
        "--leak-check=full",
        "--error-exitcode={}".format(VALGRIND_ERROR_EXITCODE),
        "--show-leak-kinds=all",
        "--errors-for-leak-kinds=all",
        "--track-origins=yes"]
    suppression_files = SUPPRESSION_FILES + custom_suppression_files
    for suppression_file in [Path(p).absolute() for p in suppression_files]:
        cmd.append("--suppressions={}".format(suppression_file))
    cmd.extend([DOWNWARD_BIN] + config + ["--internal-plan-file", PLAN_FILE])
    print("\nRun: {}".format(escape_list(cmd)))
    sys.stdout.flush()
    try:
        subprocess.check_call(cmd, stdin=open(SAS_FILE), cwd=REPO)
    except OSError as err:
        if err.errno == errno.ENOENT:
            pytest.fail(
                "Could not find valgrind. Please install it "
                "with \"sudo apt install valgrind\".")
    except subprocess.CalledProcessError as err:
        # Valgrind exits with
        #   - the exit code of the binary if it does not find leaks
        #   - VALGRIND_ERROR_EXITCODE if it finds leaks
        #   - 1 in case of usage errors
        # Fortunately, we only use exit code 1 for portfolios.
        if err.returncode == 1:
            pytest.fail("failed to run valgrind")
        elif err.returncode == VALGRIND_ERROR_EXITCODE:
            pytest.fail("{config} leaks memory for {task}".format(**locals()))


def translate(task):
    subprocess.check_call(
        [sys.executable, FAST_DOWNWARD, "--sas-file", SAS_FILE, "--translate", task],
        cwd=REPO)


def setup_module(_module):
    translate(TASK)


@pytest.mark.parametrize("config", sorted(CONFIGS.values()))
def test_configs(config, suppression_files):
    run_plan_script(SAS_FILE, config, suppression_files)


def teardown_module(_module):
    os.remove(SAS_FILE)
    os.remove(PLAN_FILE)
