#! /usr/bin/env python

from lab.parser import Parser


def add_dominance_pruning_failed(content, props):
    if "pdb_collection_construction_time" not in props:
        failed = False
    else:
        failed = "dominance_pruning_time" not in props
    props["dominance_pruning_failed"] = int(failed)


def main():
    print "Running custom parser"
    parser = Parser()
    parser.add_pattern(
        "pdb_collection_construction_time", "PDB collection construction time: (.+)s", type=float)
    parser.add_pattern(
        "dominance_pruning_time", "Dominance pruning took (.+)s", type=float)
    parser.add_pattern(
        "dominance_pruning_pruned_subsets", "Pruned (\d+) of \d+ maximal additive subsets", type=int)
    parser.add_pattern(
        "dominance_pruning_pruned_pdbs", "Pruned (\d+) of \d+ PDBs", type=int)
    parser.add_function(add_dominance_pruning_failed)
    parser.parse()


main()

