#! /usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import glob
import os.path
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
SRC_DIR = os.path.join(REPO, "src")


def check_header_files(component):
    component_dir = os.path.join(SRC_DIR, component)
    header_files = (glob.glob(os.path.join(component_dir, "*.h")) +
                    glob.glob(os.path.join(component_dir, "*", "*.h")))
    assert header_files
    errors = []
    for filename in header_files:
        assert filename.endswith(".h"), filename
        rel_filename = os.path.relpath(filename, start=component_dir)
        guard = rel_filename.replace(".", "_").replace("/", "_").replace("-", "_").upper()
        expected = "#ifndef " + guard
        for line in open(filename):
            line = line.rstrip("\n")
            if line.startswith("#ifndef"):
                if line != expected:
                    errors.append('%s uses guard "%s" but should use "%s"' %
                                  (filename, line, expected))
                break
    return errors


def main():
    errors = []
    errors.extend(check_header_files("search"))
    for error in errors:
        print(error)
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
