# -*- coding: utf-8 -*-

import os
import subprocess

from .arguments import EXAMPLES


SRC_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def test_commandline_args():
    os.chdir(SRC_DIR)
    for description, cmd in EXAMPLES:
        assert subprocess.check_call(cmd) == 0


if __name__ == "__main__":
    test_commandline_args()
