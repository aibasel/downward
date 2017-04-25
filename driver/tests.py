# -*- coding: utf-8 -*-

"""
Test module for Fast Downward driver script. Run with

    py.test driver/tests.py
"""

import os
import subprocess

from .aliases import ALIASES, PORTFOLIOS
from .arguments import EXAMPLES
from . import limits
from .returncodes import EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE
from .util import REPO_ROOT_DIR, find_domain_filename


def translate():
    """Create translated task."""
    cmd = ["./fast-downward.py", "--translate",
           "misc/tests/benchmarks/gripper/prob01.pddl"]
    assert subprocess.check_call(cmd, cwd=REPO_ROOT_DIR) == 0


def cleanup():
    subprocess.check_call(["./fast-downward.py", "--cleanup"],
                          cwd=REPO_ROOT_DIR)


def run_driver(cmd):
    cleanup()
    translate()
    return subprocess.call(cmd, cwd=REPO_ROOT_DIR)


def test_commandline_args():
    for description, cmd in EXAMPLES:
        cmd = [x.strip('"') for x in cmd]
        assert run_driver(cmd) == 0


def test_aliases():
    for alias, config in ALIASES.items():
        cmd = ["./fast-downward.py", "--alias", alias, "output.sas"]
        assert run_driver(cmd) == 0


def test_portfolios():
    for name, portfolio in PORTFOLIOS.items():
        cmd = ["./fast-downward.py", "--portfolio", portfolio,
               "--search-time-limit", "30m", "output.sas"]
        assert run_driver(cmd) in [
            EXIT_PLAN_FOUND, EXIT_UNSOLVED_INCOMPLETE]


def test_time_limits():
    for internal, external, expected_soft, expected_hard in [
            (1.5, 10, 2, 3),
            (1.0, 10, 1, 2),
            (0.5, 10, 1, 2),
            (0.5, 1, 1, 1),
            (0.5, float("inf"), 1, 2),
            (0.5, 0, 0, 0),
            ]:
        assert (limits._get_soft_and_hard_time_limits(internal, external) ==
            (expected_soft, expected_hard))


def test_automatic_domain_file_name_computation():
    benchmarks_dir = os.path.join(REPO_ROOT_DIR, "benchmarks")
    for dirpath, dirnames, filenames in os.walk(benchmarks_dir):
        for filename in filenames:
            if "domain" not in filename:
                assert find_domain_filename(os.path.join(dirpath, filename))
