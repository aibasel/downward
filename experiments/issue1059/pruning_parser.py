#! /usr/bin/env python

import re

from lab.parser import Parser

parser = Parser()
parser.add_pattern(
    "pruning_time",
    r"Time for pruning operators: (.+)s",
    type=float)
parser.add_pattern(
    "landmarks", 
    r"Landmark graph contains (\d+) landmarks, of which \d+ are disjunctive and \d+ are conjunctive.",
    type=int)
parser.add_pattern(
    "landmarks_disjunctive", 
    r"Landmark graph contains \d+ landmarks, of which (\d+) are disjunctive and \d+ are conjunctive.",
    type=int)
parser.add_pattern(
    "landmarks_conjunctive", 
    r"Landmark graph contains \d+ landmarks, of which \d+ are disjunctive and (\d+) are conjunctive.",
    type=int)
parser.add_pattern(
    "orderings",
    r"Landmark graph contains (\d+) orderings.",
    type=int)

parser.parse()
