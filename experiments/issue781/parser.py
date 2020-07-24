#! /usr/bin/env python

from lab.parser import Parser


def main():
    print 'Running custom parser'
    parser = Parser()
    parser.add_pattern('time_for_pruning_operators', r'^Time for pruning operators: (.+)s$', type=float, flags="M")
    parser.parse()


main()
