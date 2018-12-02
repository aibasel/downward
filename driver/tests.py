# -*- coding: utf-8 -*-

"""
Test module for Fast Downward driver script. Run with

    py.test driver/tests.py
"""

import os
import subprocess

import pytest

from .aliases import ALIASES, PORTFOLIOS
from .arguments import EXAMPLES
from . import limits
from . import returncodes
from .util import REPO_ROOT_DIR, find_domain_filename


def translate():
    """Create translated task."""
    cmd = ["./fast-downward.py", "--translate",
           "misc/tests/benchmarks/gripper/prob01.pddl"]
    subprocess.check_call(cmd, cwd=REPO_ROOT_DIR)


def cleanup():
    subprocess.check_call(["./fast-downward.py", "--cleanup"],
                          cwd=REPO_ROOT_DIR)


def run_driver(cmd):
    cleanup()
    translate()
    return subprocess.check_call(cmd, cwd=REPO_ROOT_DIR)


def test_commandline_args():
    for description, cmd in EXAMPLES:
        cmd = [x.strip('"') for x in cmd]
        run_driver(cmd)


def test_aliases():
    for alias, config in ALIASES.items():
        cmd = ["./fast-downward.py", "--alias", alias, "output.sas"]
        run_driver(cmd)


def test_portfolios():
    for name, portfolio in PORTFOLIOS.items():
        cmd = ["./fast-downward.py", "--portfolio", portfolio,
               "--search-time-limit", "30m", "output.sas"]
        run_driver(cmd)


def test_hard_time_limit():
    def preexec_fn():
        limits.set_time_limit(10)

    cmd = [
        "./fast-downward.py", "--translate", "--translate-time-limit",
        "10s", "misc/tests/benchmarks/gripper/prob01.pddl"]
    subprocess.check_call(cmd, preexec_fn=preexec_fn, cwd=REPO_ROOT_DIR)

    cmd = [
        "./fast-downward.py", "--translate", "--translate-time-limit",
        "20s", "misc/tests/benchmarks/gripper/prob01.pddl"]
    with pytest.raises(subprocess.CalledProcessError) as exception_info:
        subprocess.check_call(cmd, preexec_fn=preexec_fn, cwd=REPO_ROOT_DIR)
    assert exception_info.value.returncode == returncodes.DRIVER_INPUT_ERROR


def test_automatic_domain_file_name_computation():
    benchmarks_dir = os.path.join(REPO_ROOT_DIR, "benchmarks")
    for dirpath, dirnames, filenames in os.walk(benchmarks_dir):
        for filename in filenames:
            if "domain" not in filename:
                assert find_domain_filename(os.path.join(dirpath, filename))
