import sys


printed_warnings = set()

def print_warning(msg):
    if msg not in printed_warnings:
        print(f"Warning: {msg}", file=sys.stderr)
        printed_warnings.add(msg)
