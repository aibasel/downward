#! /usr/bin/env python3

"""
Run uncrustify on all C++ files in the repository.
"""

import argparse
import errno
import os.path
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")

import utils


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-m", "--modify", action="store_true",
        help="modify the files that need to be uncrustified")
    return parser.parse_args()


def main():
    args = parse_args()
    src_files = utils.get_src_files(
        REPO, (".h", ".cc"), ignore_dirs=["builds", "data", "venv", ".venv"])
    print(f"Checking {len(src_files)} files with uncrustify.")
    config_file = os.path.join(REPO, ".uncrustify.cfg")
    executable = "uncrustify"
    cmd = [executable, "-q", "-c", config_file] + src_files
    if args.modify:
        cmd.append("--no-backup")
    else:
        cmd.append("--check")
    try:
        # Hide clean files printed on stdout.
        returncode = subprocess.call(cmd, stdout=subprocess.PIPE)
    except OSError as err:
        if err.errno == errno.ENOENT:
            sys.exit(f"Error: {executable} not found. Is it on the PATH?")
        else:
            raise
    if returncode != 0:
        print('Run "tox -e fix-style" in the misc/ directory to fix the C++ style.')
    return returncode


if __name__ == "__main__":
    sys.exit(main())
