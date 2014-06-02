#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os.path

for filename in sys.argv[1:]:
    assert filename.endswith(".h"), filename
    guard = filename.replace(".", "_").replace("/", "_").replace("-", "_").upper()
    expected = "#ifndef " + guard
    for line in open(filename):
        line = line.rstrip("\n")
        if line.startswith("#ifndef"):
            print line == expected, filename, line
