#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('single_cegar_pdbs_computation_time', 'CEGAR_PDBs: computation time: (.+)s', required=False, type=float)
parser.add_pattern('single_cegar_pdbs_num_iterations', 'CEGAR_PDBs: number of iterations: (\d+)', required=False, type=int)
parser.add_pattern('single_cegar_pdbs_collection_num_patterns', 'CEGAR_PDBs: final collection number of patterns: (.+)', required=False, type=int)
parser.add_pattern('single_cegar_pdbs_collection_summed_pdb_size', 'CEGAR_PDBs: final collection summed PDB sizes: (.+)', required=False, type=int)

def parse_lines(content, props):
    single_cegar_pdbs_timed_out = False
    single_cegar_pdbs_solved_without_search = False
    for line in content.split('\n'):
        if line == 'CEGAR_PDBs: time limit reached':
            single_cegar_pdbs_timed_out = True
        if line == 'CEGAR_PDBs: task solved during computation of abstract solutions':
            single_cegar_pdbs_solved_without_search = True
    props['single_cegar_pdbs_timed_out'] = single_cegar_pdbs_timed_out
    props['single_cegar_pdbs_solved_without_search'] = single_cegar_pdbs_solved_without_search

parser.add_function(parse_lines)

parser.parse()
