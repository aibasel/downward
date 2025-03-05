import sys


PRINTED_WARNINGS = set()

def print_warning(msg):
    global PRINTED_WARNINGS
    if not msg in PRINTED_WARNINGS:
        print(f"Warning: {msg}", file=sys.stderr)
        PRINTED_WARNINGS.add(msg)
