#! /usr/bin/env python

from __future__ import print_function

import os
import subprocess
import sys

import configs

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
BENCHMARKS_DIR = os.path.join(REPO, "misc", "tests", "benchmarks")
FAST_DOWNWARD = os.path.join(REPO, "fast-downward.py")

TASKS = [os.path.join(BENCHMARKS_DIR, path) for path in [
    "miconic/s1-0.pddl",
]]

CONFIGS = {}
CONFIGS.update(configs.default_configs_optimal(core=True, ipc=True, extended=True))
CONFIGS.update(configs.default_configs_satisficing(core=True, ipc=True, extended=True))
CONFIGS.update(configs.task_transformation_test_configs())
CONFIGS.update(configs.regression_test_configs())

if os.name == "nt":
    # No support for portfolios on Windows
    del CONFIGS["seq_opt_merge_and_shrink"]
    del CONFIGS["seq_opt_fdss_1"]
    del CONFIGS["seq_opt_fdss_2"]
    del CONFIGS["seq_sat_lama_2011"]
    del CONFIGS["seq_sat_fdss_1"]
    del CONFIGS["seq_sat_fdss_2"]

def run_plan_script(task, nick, config, debug):
    cmd = [sys.executable, FAST_DOWNWARD]
    if os.name != "nt":
        cmd.extend(["--search-time-limit", "30m"])
    if debug:
        cmd.append("--debug")
    if "--alias" in config:
        assert len(config) == 2, config
        cmd += config + [task]
    else:
        cmd += [task] + config
    print("\nRun {}:".format(cmd))
    sys.stdout.flush()
    subprocess.check_call(cmd)


def cleanup():
    subprocess.check_call([sys.executable, FAST_DOWNWARD, "--cleanup"])


def main():
    # On Windows, ./build.py has to be called from the correct environment.
    # Since we want this script to work even when we are in a regular
    # shell, we do not build on Windows. If the planner is not yet built,
    # the driver script will complain about this.
    if os.name == "posix":
        subprocess.check_call(["./build.py", "release32", "debug32"], cwd=REPO)
    for task in TASKS:
        for nick, config in CONFIGS.items():
            for debug in [False, True]:
                try:
                    run_plan_script(task, nick, config, debug)
                except subprocess.CalledProcessError:
                    sys.exit(
                        "\nError: {} failed to solve {} (debug={})".format(
                            nick, task, debug))
                cleanup()

main()
