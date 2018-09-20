#! /usr/bin/env python

from lab.parser import Parser


def main():
    parser = Parser()
    parser.add_pattern(
        "int_hash_set_load_factor",
        "Int hash set load factor: \d+/\d+ = (.+)",
        required=False,
        type=float)
    parser.add_pattern(
        "int_hash_set_resizes",
        "Int hash set resizes: (\d+)",
        required=False,
        type=int)
    print "Running custom parser"
    parser.parse()

main()
