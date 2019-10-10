from __future__ import print_function

import os
import pipes
import subprocess
import sys

import pytest

import configs

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
BENCHMARKS_DIR = os.path.join(REPO, "misc", "tests", "benchmarks")
FAST_DOWNWARD = os.path.join(REPO, "fast-downward.py")
SAS_FILE = os.path.join(REPO, "test.sas")
PLAN_FILE = os.path.join(REPO, "test.plan")
TASK = os.path.join(BENCHMARKS_DIR, "miconic/s1-0.pddl")

CONFIGS = {}
CONFIGS.update(configs.default_configs_optimal(core=True, extended=True))
CONFIGS.update(configs.default_configs_satisficing(core=True, extended=True))


def escape_list(l):
    return " ".join(pipes.quote(x) for x in l)


def run_plan_script(task, config, debug):
    cmd = [sys.executable, FAST_DOWNWARD, "--plan-file", PLAN_FILE]
    if debug:
        cmd.append("--debug")
    if "--alias" in config:
        assert len(config) == 2, config
        cmd += config + [task]
    else:
        cmd += [task] + config
    print("\nRun: {}:".format(escape_list(cmd)))
    sys.stdout.flush()
    subprocess.check_call(cmd, cwd=REPO)


def translate(task):
    subprocess.check_call([
        sys.executable, FAST_DOWNWARD, "--sas-file", SAS_FILE, "--translate", task], cwd=REPO)


def cleanup():
    os.remove(SAS_FILE)
    os.remove(PLAN_FILE)


def setup_module(module):
    translate(TASK)


@pytest.mark.parametrize("config", sorted(CONFIGS.values()))
@pytest.mark.parametrize("debug", [False, True])
def test_configs(config, debug):
    run_plan_script(SAS_FILE, config, debug)


def teardown_module(module):
    cleanup()
