#! /usr/bin/env python

import re

from lab.parser import Parser

parser = Parser()
parser.add_pattern(
    "landmarks", 
    r"Discovered (\d+) landmarks, of which \d+ are disjunctive and \d+ are conjunctive."
)
parser.add_pattern(
    "disjunctive_landmarks", 
    r"Discovered \d+ landmarks, of which (\d+) are disjunctive and \d+ are conjunctive."
)
parser.add_pattern(
    "conjunctive_landmarks", 
    r"Discovered \d+ landmarks, of which \d+ are disjunctive and (\d+) are conjunctive."
)
parser.add_pattern("orderings", r"(\d+) edges")

parser.parse()
