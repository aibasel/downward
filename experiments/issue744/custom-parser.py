#! /usr/bin/env python

from lab.parser import Parser

def compute_log_size(content, props):
    props["log_size"] = len(content)

def main():
    parser = Parser()
    parser.add_function(compute_log_size)
    print "Running custom parser"
    parser.parse()

main()
