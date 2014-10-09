# -*- coding: utf-8 -*-

import os
import subprocess

import pytest

from .aliases import ALIASES, PORTFOLIOS
from .arguments import EXAMPLES
from .portfolio_runner import EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE


SRC_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


@pytest.fixture
def preprocess():
    """Create preprocessed task."""
    os.chdir(SRC_DIR)
    cmd = ["./plan.py", "--run-translator", "--run-preprocessor",
           "../benchmarks/gripper/prob01.pddl"]
    assert subprocess.check_call(cmd) == 0


def test_commandline_args(preprocess):
    for description, cmd in EXAMPLES:
        assert subprocess.check_call(cmd) == 0


def test_aliases(preprocess):
    for alias, config in ALIASES.items():
        # selmax is currently not supported.
        if alias in ["seq-opt-fd-autotune", "seq-opt-selmax"]:
            continue
        cmd = ["./plan.py", "--alias", alias, "output"]
        assert subprocess.check_call(cmd) == 0


def test_portfolios(preprocess):
    for name, portfolio in PORTFOLIOS.items():
        if name in ["seq-opt-fd-autotune", "seq-opt-selmax"]:
            continue
        cmd = ["./plan.py", "--portfolio", portfolio, "output"]
        assert subprocess.call(cmd) in [
            EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE]
