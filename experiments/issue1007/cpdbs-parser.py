#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('cpdbs_num_patterns', 'Canonical PDB heuristic number of patterns: (\d+)', required=False, type=int)
parser.add_pattern('cpdbs_total_pdb_size', 'Canonical PDB heuristic total PDB size: (\d+)', required=False, type=int)
parser.add_pattern('cpdbs_computation_time', 'Canonical PDB heuristic computation time: (.+)s', required=False, type=float)

parser.parse()
