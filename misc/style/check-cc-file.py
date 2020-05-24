#! /usr/bin/env python3


import argparse
import re
import subprocess
import sys


STD_REGEX = r"(^|\s|\W)std::"

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("cc_file", nargs="+")
    return parser.parse_args()


def check_std(cc_file):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    errors = []
    for line in source_without_comments.splitlines():
        if re.search(STD_REGEX, line):
            errors.append("Remove std:: from {}: {}".format(
                cc_file, line.strip()))
    return errors


def main():
    args = parse_args()

    errors = []
    for cc_file in args.cc_file:
        errors.extend(check_std(cc_file))
    for error in errors:
        print(error)
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
