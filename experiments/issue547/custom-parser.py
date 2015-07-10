#! /usr/bin/env python

from lab.parser import Parser


class CustomParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        self.add_pattern(
            "successor_generator_time",
            "Building successor generator...done! \[t=(.+)s\]",
            required=False,
            type=float)


if __name__ == "__main__":
    parser = CustomParser()
    print "Running custom parser"
    parser.parse()
