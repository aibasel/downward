#! /usr/bin/env python

from lab.parser import Parser


def main():
    print "Running custom parser"
    parser = Parser()
    parser.add_pattern(
        "dominance_pruning_time", "Dominance pruning took (.+)s", type=float)
    parser.add_pattern(
        "dominance_pruning_pruned_subsets", "Pruned (\d+) of \d+ maximal additive subsets", type=int)
    parser.add_pattern(
        "dominance_pruning_pruned_pdbs", "Pruned (\d+) of \d+ PDBs", type=int)
    parser.parse()


main()

