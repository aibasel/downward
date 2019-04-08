#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('generator_computation_time', 'generator computation time: (.+)s', required=False, type=float)
parser.add_pattern('cpdbs_computation_time', 'Canonical PDB heuristic computation time: (.+)s', required=False, type=float)
parser.add_pattern('dominance_pruning_time', 'Dominance pruning took (.+)s', required=False, type=float)

parser.parse()
