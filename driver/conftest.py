# This file is read by pytest before running the tests to set them up. We use it
# to have the SAS+ file for the example task ready for the @pytest.parametrize
# function in tests.py. Translating the task in setup_module() does not work in
# this case, because the @pytest.parametrize decorator is executed before
# setup_module() is called.

import subprocess
import sys

from .util import REPO_ROOT_DIR


def translate():
    """Create output.sas file for example task."""
    cmd = [sys.executable, "fast-downward.py", "--translate",
           "misc/tests/benchmarks/gripper/prob01.pddl"]
    subprocess.check_call(cmd, cwd=REPO_ROOT_DIR)


def pytest_sessionstart(session):
    translate()
