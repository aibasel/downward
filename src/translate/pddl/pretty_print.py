from __future__ import print_function

import io
import textwrap

__all__ = ["print_nested_list"]

def tokenize_list(obj):
    if isinstance(obj, list):
        yield "("
        for item in obj:
            for elem in tokenize_list(item):
                yield elem
        yield ")"
    else:
        yield obj

def wrap_lines(lines):
    for line in lines:
        indent = " " * (len(line) - len(line.lstrip()) + 4)
        line = line.replace("-", "_") # textwrap breaks on "-", but not "_"
        line = textwrap.fill(line, subsequent_indent=indent, break_long_words=False)
        yield line.replace("_", "-")

def print_nested_list(nested_list):
    stream = io.StringIO()
    indent = 0
    startofline = True
    pendingspace = False
    for token in tokenize_list(nested_list):
        if token == "(":
            if not startofline:
                stream.write("\n")
            stream.write("%s(" % (" " * indent))
            indent += 2
            startofline = False
            pendingspace = False
        elif token == ")":
            indent -= 2
            stream.write(")")
            startofline = False
            pendingspace = False
        else:
            if startofline:
                stream.write(" " * indent)
            if pendingspace:
                stream.write(" ")
            stream.write(token)
            startofline = False
            pendingspace = True

    for line in wrap_lines(stream.getvalue().splitlines()):
        print(line)
