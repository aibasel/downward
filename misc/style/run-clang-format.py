#! /usr/bin/env python3

"""
Run clang-format on all C++ files in the repository.
"""

import argparse
import os.path
import subprocess
import sys

import utils

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SEARCH_DIR = os.path.join(REPO, "src", "search")
CLANG_FORMAT_VERSION = "18"


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-m", "--modify", action="store_true",
        help="modify the files that need to be clang-formatted")
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


def get_clang_format_version():
    try:
        result = subprocess.run(
            [f"clang-format-{CLANG_FORMAT_VERSION}", "--version"],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        return f"clang-format-{CLANG_FORMAT_VERSION} not found"


def main():
    args = parse_args()
    if not args.force and args.modify and search_files_are_dirty():
        sys.exit(f"Error: {SEARCH_DIR} has uncommited changes.")
    src_files = utils.get_src_files(SEARCH_DIR, (".h", ".cc"))
    print(f"Checking {len(src_files)} files with clang-format.")
    config_file = os.path.join(REPO, ".clang-format")
    executable = f"clang-format-{CLANG_FORMAT_VERSION}"
    exe_error_str = f"Error: {executable} not found. Is it on the PATH?"
    flag = "-i" if args.modify else "--dry-run"
    cmd = [
        executable, flag, "--Werror", f"--style=file:{config_file}"
    ] + src_files
    try:
        # Hide clean files printed on stdout.
        returncode = subprocess.call(cmd, stdout=subprocess.PIPE)
    except FileNotFoundError as not_found:
        src_error_str = f"Error: Did not find file: '{not_found.filename}'."
        error_str = exe_error_str if not_found == executable else src_error_str
        sys.exit(error_str)
    if not args.modify and returncode != 0:
        version = get_clang_format_version()
        print(f"Format issue detected by: {version}")
        print('Run "tox -e fix-style" in the misc/ directory to fix the C++ ' +
            'style.')
    return returncode


if __name__ == "__main__":
    sys.exit(main())
