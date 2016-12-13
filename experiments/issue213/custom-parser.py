#! /usr/bin/env python

from lab.parser import Parser


def main():
    parser = Parser()
    parser.add_pattern(
        "hash_set_load_factor",
        "Hash set load factor: \d+/\d+ = (.+)",
        required=False,
        type=float)
    parser.add_pattern(
        "hash_set_resizings",
        "Hash set resizings: (\d+)",
        required=False,
        type=int)
    print "Running custom parser"
    parser.parse()

main()
