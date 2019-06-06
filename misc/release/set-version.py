#!/usr/bin/env python3
"""
Convenience script to change the version number in the driver
"""

import os
import sys

from utils import fill_template

def main(tag):
    repo_base = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../..")
    version_file = os.path.join(repo_base, "driver", "version.py")

    fill_template("_version", version_file, dict(TAG=tag))

if __name__ == "__main__":
    main(sys.argv[1])

# TODO
# - commit change
