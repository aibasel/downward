#! /usr/bin/env python

import re

from lab.parser import Parser

parser = Parser()
parser.add_pattern("lm_graph_generation_time", r"Landmark graph generation time: (.+)s")
parser.add_pattern(
    "landmarks", 
    r"Landmark graph contains (\d+) landmarks, of which \d+ are disjunctive and \d+ are conjunctive."
)
parser.add_pattern(
    "disjunctive_landmarks", 
    r"Landmark graph contains \d+ landmarks, of which (\d+) are disjunctive and \d+ are conjunctive."
)
parser.add_pattern(
    "conjunctive_landmarks", 
    r"Landmark graph contains \d+ landmarks, of which \d+ are disjunctive and (\d+) are conjunctive."
)
parser.add_pattern("orderings", r"Landmark graph contains (\d+) orderings.")

parser.parse()
