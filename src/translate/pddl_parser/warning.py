import sys


PRINTED_WARNINGS = set()

def print_warning(msg):
    global PRINTED_WARNINGS
    if msg not in PRINTED_WARNINGS:
        print(f"Warning: {msg}", file=sys.stderr)
        PRINTED_WARNINGS.add(msg)
