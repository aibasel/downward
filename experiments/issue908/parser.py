#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('computation_time', 'computation time: (.+)s', required=False, type=float)

parser.parse()
