#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
# This matches output of both single and multiple random pattern algorithms.
parser.add_pattern('random_pattern_num_iterations', 'Random Pattern.* number of iterations: (\d+)', required=False, type=int)
parser.add_pattern('random_pattern_num_patterns', 'Random Pattern.* number of patterns: (\d+)', required=False, type=int)
parser.add_pattern('random_pattern_total_pdb_size', 'Random Pattern.* total PDB size: (\d+)', required=False, type=int)
parser.add_pattern('random_pattern_computation_time', 'Random Pattern.* computation time: (.+)s', required=False, type=float)

parser.parse()
