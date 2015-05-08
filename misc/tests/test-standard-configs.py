#! /usr/bin/env python

import os
import subprocess
import sys

import configs

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
BENCHMARKS_DIR = os.path.join(REPO, "benchmarks")
SRC_DIR = os.path.join(REPO, "src")
FAST_DOWNWARD = os.path.join(SRC_DIR, "fast-downward.py")

TASKS = [os.path.join(BENCHMARKS_DIR, path) for path in [
    "miconic/s1-0.pddl",
]]

CONFIGS = {}
CONFIGS.update(configs.default_configs_optimal(core=True, ipc=False, extended=True))
CONFIGS.update(configs.default_configs_satisficing(core=True, ipc=False, extended=True))
CONFIGS.update(configs.task_transformation_test_configs())

if "astar_selmax_lmcut_lmcount" in CONFIGS:
    del CONFIGS["astar_selmax_lmcut_lmcount"]


def run_plan_script(task, nick, config):
    print "\nRun %(nick)s on %(task)s:" % locals()
    sys.stdout.flush()
    subprocess.check_call([FAST_DOWNWARD, task] + config)


def cleanup():
    subprocess.check_call([os.path.join(SRC_DIR, "cleanup")])


def main():
    subprocess.check_call(["./build_all"], cwd=SRC_DIR)
    failures = []
    for task in TASKS:
        for nick, config in CONFIGS.items():
            try:
                run_plan_script(task, nick, config)
            except subprocess.CalledProcessError:
                sys.exit("\nError: %(nick)s failed to solve task %(task)s" % locals())
    cleanup()

main()
