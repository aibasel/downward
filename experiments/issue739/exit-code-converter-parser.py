#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()

LEGACY_TO_NEW_EXIT_CODES = {
    'critical-error': 'search-critical-error',
    'input-error': 'search-input-error',
    'unsupported-feature-requested': 'search-unsupported',
    'unsolvable': 'search-unsolvable',
    'incomplete-search-found-no-plan': 'search-unsolvable-incomplete',
    'out-of-memory': 'search-out-of-memory',
    'timeout': 'search-out-of-time',
    'timeout-and-out-of-memory': 'search-out-of-memory-and-time',
}

def convert_legacy_to_new_exit_codes(content, props):
    error = props['error']
    if error in LEGACY_TO_NEW_EXIT_CODES:
        props['error'] = LEGACY_TO_NEW_EXIT_CODES[error]

parser.add_function(convert_legacy_to_new_exit_codes)

parser.parse()
