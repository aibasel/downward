#! /usr/bin/env python

"""
========================================================================
Simplifying 29928 unary operators... done! [17292 unary operators]
time to simplify: 0.022623s
========================================================================

=> Here we want to extract 29928 (simplify_before), 17292 (simplify_after) and
0.022623s (simplify_time).
"""

import re

from lab.parser import Parser


print 'Running custom parser'
parser = Parser()
parser.add_pattern('simplify_before', r'^Simplifying (\d+) unary operators\.\.\. done! \[\d+ unary operators\]$', type=int)
parser.add_pattern('simplify_after', r'^Simplifying \d+ unary operators\.\.\. done! \[(\d+) unary operators\]$', type=int)
parser.add_pattern('simplify_time', r'^time to simplify: (.+)s$', type=float)
parser.parse()
