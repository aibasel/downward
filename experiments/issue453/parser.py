#! /usr/bin/env python

import logging
import re

from lab.parser import Parser

def main():
    parser = Parser()
    parser.add_pattern("translator_derived_variables", r"Translator derived variables: (\d+)", type=int)
    parser.add_pattern("translator_axioms", r"Translator axioms: (\d+)", type=int)

    parser.parse()


if __name__ == "__main__":
    main()
