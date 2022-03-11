#! /usr/bin/env python

import re

from lab.parser import Parser

parser = Parser()

def count_dead_ends(content, props):
    empty_first_matches_v1 = re.findall(r"Found unreached landmark with empty first achievers.", content)
    empty_first_matches_v2 = re.findall(r"Found unreached landmark in initial state with empty first achievers.", content)
    props["dead-end_empty_first_achievers"] = len(empty_first_matches_v1) + len(empty_first_matches_v2)

    empty_possible_matches = re.findall(r"Found needed-again landmark with empty possible achievers.", content)
    props["dead-end_empty_possible_achievers"] = len(empty_possible_matches)

    return props

parser.add_function(count_dead_ends)

parser.parse()
