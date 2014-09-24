#! /usr/bin/env python

from lab.parser import Parser

class RawMemoryParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        self.add_pattern('raw_memory', r'Peak memory: (.+) KB', type=int, required=False)


if __name__ == '__main__':
    parser = RawMemoryParser()
    print 'Running RawMemoryParser parser'
    parser.parse()

