#! /usr/bin/env python

from lab.parser import Parser

print 'Running axiom evaluation time parser'
parser = Parser()
parser.add_pattern('axiom_time_inner', r'AxiomEvaluator time in inner evaluate: (.+)', type=float)
parser.add_pattern('axiom_time_outer', r'AxiomEvaluator time in outer evaluate: (.+)', type=float)

parser.parse()
