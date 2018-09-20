#!/usr/bin/env python

import hashlib

from lab.parser import Parser

def add_hash_value(content, props):
    props['translator_output_sas_xz_hash'] = hashlib.sha512(content).hexdigest()

parser = Parser()
parser.add_function(add_hash_value, file="output.sas.xz")
parser.parse()
