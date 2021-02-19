"""
Test module for Fast Downward driver script. Run with

    py.test driver/tests.py
"""

import os
import subprocess
import sys

import pytest

from .aliases import ALIASES, PORTFOLIOS
from .arguments import EXAMPLES
from . import limits
from . import returncodes
from .util import REPO_ROOT_DIR, find_domain_filename


def translate():
    """Create translated task."""
    cmd = [sys.executable, "fast-downward.py", "--translate",
           "misc/tests/benchmarks/gripper/prob01.pddl"]
    subprocess.check_call(cmd, cwd=REPO_ROOT_DIR)


def cleanup():
    subprocess.check_call([sys.executable, "fast-downward.py", "--cleanup"],
                          cwd=REPO_ROOT_DIR)


def run_driver(parameters):
    cleanup()
    translate()
    cmd = [sys.executable, "fast-downward.py"] + parameters
    return subprocess.check_call(cmd, cwd=REPO_ROOT_DIR)


def test_commandline_args():
    for description, cmd in EXAMPLES:
        parameters = [x.strip('"') for x in cmd]
        run_driver(parameters)


def test_aliases():
    for alias, config in ALIASES.items():
        parameters = ["--alias", alias, "output.sas"]
        run_driver(parameters)


def test_show_aliases():
    run_driver(["--show-aliases"])


def test_portfolios():
    for name, portfolio in PORTFOLIOS.items():
        parameters = ["--portfolio", portfolio,
                      "--search-time-limit", "30m", "output.sas"]
        run_driver(parameters)


@pytest.mark.skipif(not limits.can_set_time_limit(), reason="Cannot set time limits on this system")
def test_hard_time_limit():
    def preexec_fn():
        limits.set_time_limit(10)

    driver = [sys.executable, "fast-downward.py"]
    parameters = [
        "--translate", "--translate-time-limit",
        "10s", "misc/tests/benchmarks/gripper/prob01.pddl"]
    subprocess.check_call(driver + parameters, preexec_fn=preexec_fn, cwd=REPO_ROOT_DIR)

    parameters = [
        "--translate", "--translate-time-limit",
        "20s", "misc/tests/benchmarks/gripper/prob01.pddl"]
    with pytest.raises(subprocess.CalledProcessError) as exception_info:
        subprocess.check_call(driver + parameters, preexec_fn=preexec_fn, cwd=REPO_ROOT_DIR)
    assert exception_info.value.returncode == returncodes.DRIVER_INPUT_ERROR


def test_automatic_domain_file_name_computation():
    benchmarks_dir = os.path.join(REPO_ROOT_DIR, "benchmarks")
    for dirpath, dirnames, filenames in os.walk(benchmarks_dir):
        for filename in filenames:
            if "domain" not in filename:
                assert find_domain_filename(os.path.join(dirpath, filename))
