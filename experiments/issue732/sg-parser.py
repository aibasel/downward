#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()

parser.add_pattern('sg_construction_time', 'time for root successor generation creation: (.+)s', type=float)
parser.add_pattern('sg_peak_mem_diff', 'peak memory difference for root successor generator creation: (\d+) KB', type=int)

parser.parse()
