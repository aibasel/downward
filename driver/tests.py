# -*- coding: utf-8 -*-

"""
Test module for Fast Downward driver script. Run with

    py.test driver/tests.py
"""

import subprocess

from .aliases import ALIASES, PORTFOLIOS
from .arguments import EXAMPLES
from .portfolio_runner import EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE
from .util import PLANNER_ROOT_DIR


def preprocess():
    """Create preprocessed task."""
    cmd = ["./fast-downward.py", "--translate", "--preprocess",
           "../benchmarks/gripper/prob01.pddl"]
    assert subprocess.check_call(cmd, cwd=PLANNER_ROOT_DIR) == 0


def cleanup():
    subprocess.check_call("./cleanup.py", cwd=PLANNER_ROOT_DIR)


def run_driver(cmd):
    cleanup()
    preprocess()
    return subprocess.call(cmd, cwd=SRC_DIR)


def test_commandline_args():
    for description, cmd in EXAMPLES:
        cmd = [x.strip('"') for x in cmd]
        assert run_driver(cmd) == 0


def test_aliases():
    for alias, config in ALIASES.items():
        # selmax is currently not supported.
        if alias in ["seq-opt-fd-autotune", "seq-opt-selmax"]:
            continue
        cmd = ["./fast-downward.py", "--alias", alias, "output"]
        assert run_driver(cmd) == 0


def test_portfolios():
    for name, portfolio in PORTFOLIOS.items():
        if name in ["seq-opt-fd-autotune", "seq-opt-selmax"]:
            continue
        cmd = ["./fast-downward.py", "--portfolio", portfolio,
               "--search-time-limit", "30m", "output"]
        assert run_driver(cmd) in [
            EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE]
