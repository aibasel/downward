#! /usr/bin/env python

from lab.parser import Parser


class CustomParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        self.add_pattern(
            "num_sga_patterns",
            "Found (\d+) SGA patterns.",
            required=False,
            type=int)
        self.add_pattern(
            "num_interesting_patterns",
            "Found (\d+) interesting patterns.",
            required=False,
            type=int)


if __name__ == "__main__":
    parser = CustomParser()
    print "Running custom parser"
    parser.parse()
