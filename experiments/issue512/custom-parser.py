#! /usr/bin/env python

from lab.parser import Parser


class CustomParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        self.add_pattern(
            "init_time",
            "Best heuristic value: \d+ \[g=0, 1 evaluated, 0 expanded, t=(.+)s, \d+ KB\]",
            required=True,
            type=float)


if __name__ == "__main__":
    parser = CustomParser()
    print "Running custom parser"
    parser.parse()
