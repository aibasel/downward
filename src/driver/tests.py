# -*- coding: utf-8 -*-

"""
Test module for Fast Downward driver script. Run with

    py.test driver/tests.py
"""

import os
import subprocess

from .aliases import ALIASES, PORTFOLIOS
from .arguments import EXAMPLES
from .portfolio_runner import EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE


SRC_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def preprocess():
    """Create preprocessed task."""
    os.chdir(SRC_DIR)
    cmd = ["./fast-downward.py", "--translate", "--preprocess",
           "../benchmarks/gripper/prob01.pddl"]
    assert subprocess.check_call(cmd) == 0


def cleanup():
    subprocess.check_call("./cleanup", cwd=SRC_DIR)


def run_driver(cmd):
    cleanup()
    preprocess()
    return subprocess.call(cmd)


def test_commandline_args():
    for description, cmd in EXAMPLES:
        if "--portfolio" in cmd:
            continue
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
        cmd = ["./fast-downward.py", "--portfolio", portfolio, "output"]
        assert run_driver(cmd) in [
            EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE]
