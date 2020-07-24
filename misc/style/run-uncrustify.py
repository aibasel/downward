#! /usr/bin/env python3

"""
Run uncrustify on all C++ files in the repository.
"""

import argparse
import os.path
import subprocess
import sys

import utils

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SEARCH_DIR = os.path.join(REPO, "src", "search")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-m", "--modify", action="store_true",
        help="modify the files that need to be uncrustified")
    parser.add_argument(
        "-f", "--force", action="store_true",
        help="modify files even if there are uncommited changes")
    return parser.parse_args()


def search_files_are_dirty():
    if os.path.exists(os.path.join(REPO, ".git")):
        cmd = ["git", "status", "--porcelain", SEARCH_DIR]
    elif os.path.exists(os.path.join(REPO, ".hg")):
        cmd = ["hg", "status", SEARCH_DIR]
    else:
        sys.exit("Error: repo must contain a .git or .hg directory.")
    return bool(subprocess.check_output(cmd, cwd=REPO))


def main():
    args = parse_args()
    if not args.force and args.modify and search_files_are_dirty():
        sys.exit(f"Error: {SEARCH_DIR} has uncommited changes.")
    src_files = utils.get_src_files(SEARCH_DIR, (".h", ".cc"))
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
    except FileNotFoundError:
        sys.exit(f"Error: {executable} not found. Is it on the PATH?")
    if not args.modify and returncode != 0:
        print('Run "tox -e fix-style" in the misc/ directory to fix the C++ style.')
    return returncode


if __name__ == "__main__":
    sys.exit(main())
