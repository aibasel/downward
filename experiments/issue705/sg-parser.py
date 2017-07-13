#! /usr/bin/env python

from lab.parser import Parser

def add_absolute_and_relative(parser, attribute, pattern):
    parser.add_pattern(attribute, pattern + ' (\d+) .+', required=False, type=int)
    parser.add_pattern(attribute + '_rel', pattern + ' \d+ \((.+)\)', required=False, type=float)


parser = Parser()

parser.add_pattern('sg_construction_time', 'SG construction time: (.+)s', required=False, type=float)
parser.add_pattern('sg_peak_mem_diff', 'SG construction peak memory difference: (\d+)', required=False, type=int)
parser.add_pattern('sg_size_estimate_total', 'SG size estimates: total: (\d+)', required=False, type=int)
add_absolute_and_relative(parser, 'sg_size_estimate_overhead', 'SG size estimates: object overhead:')
add_absolute_and_relative(parser, 'sg_size_estimate_operators', 'SG size estimates: operators:')
add_absolute_and_relative(parser, 'sg_size_estimate_switch_var', 'SG size estimates: switch var:')
add_absolute_and_relative(parser, 'sg_size_estimate_value_generator', 'SG size estimates: generator for value:')
add_absolute_and_relative(parser, 'sg_size_estimate_default_generator', 'SG size estimates: default generator:')
add_absolute_and_relative(parser, 'sg_size_estimate_next_generator', 'SG size estimates: next generator:')

add_absolute_and_relative(parser, 'sg_counts_immediates', 'SG object counts: immediates:')
add_absolute_and_relative(parser, 'sg_counts_forks', 'SG object counts: forks:')
add_absolute_and_relative(parser, 'sg_counts_switches', 'SG object counts: switches:')
add_absolute_and_relative(parser, 'sg_counts_leaves', 'SG object counts: leaves:')
add_absolute_and_relative(parser, 'sg_counts_empty', 'SG object counts: empty:')

add_absolute_and_relative(parser, 'sg_counts_switch_empty', 'SG switch statistics: immediate ops empty:')
add_absolute_and_relative(parser, 'sg_counts_switch_single', 'SG switch statistics: single immediate op:')
add_absolute_and_relative(parser, 'sg_counts_switch_more', 'SG switch statistics: more immediate ops:')

add_absolute_and_relative(parser, 'sg_counts_leaf_empty', 'SG leaf statistics: applicable ops empty:')
add_absolute_and_relative(parser, 'sg_counts_leaf_single', 'SG leaf statistics: single applicable op:')
add_absolute_and_relative(parser, 'sg_counts_leaf_more', 'SG leaf statistics: more applicable ops:')

add_absolute_and_relative(parser, 'sg_counts_switch_vector_single', 'SG switch statistics: vector single:')
add_absolute_and_relative(parser, 'sg_counts_switch_vector_small', 'SG switch statistics: vector small:')
add_absolute_and_relative(parser, 'sg_counts_switch_vector_large', 'SG switch statistics: vector large:')
add_absolute_and_relative(parser, 'sg_counts_switch_vector_full', 'SG switch statistics: vector full:')

parser.parse()
