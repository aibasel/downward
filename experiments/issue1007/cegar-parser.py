#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('cegar_num_iterations', 'CEGAR number of iterations: (\d+)', required=False, type=int)
parser.add_pattern('cegar_num_patterns', 'CEGAR number of patterns: (\d+)', required=False, type=int)
parser.add_pattern('cegar_total_pdb_size', 'CEGAR total PDB size: (\d+)', required=False, type=int)
parser.add_pattern('cegar_computation_time', 'CEGAR computation time: (.+)s', required=False, type=float)

parser.parse()
