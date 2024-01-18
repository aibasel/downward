__all__ = ["parse_nested_list"]

from .parse_error import ParseError

# Basic functions for parsing PDDL (Lisp) files.
def parse_nested_list(input_file):
    tokens = tokenize(input_file)
    next_token = next(tokens)
    if next_token != "(":
        raise ParseError(f"Expected '(', got '{next_token}'.")
    result = list(parse_list_aux(tokens))
    remaining_tokens = list(tokens)
    if remaining_tokens:
        raise ParseError(f"Tokens remaining after parsing: "
                         f"{' '.join(remaining_tokens)}")
    return result

def tokenize(input):
    for line in input:
        line = line.split(";", 1)[0]  # Strip comments.
        try:
            line.encode("ascii")
        except UnicodeEncodeError:
            raise ParseError(f"Non-ASCII character outside comment: {line[0:-1]}")
        line = line.replace("(", " ( ").replace(")", " ) ").replace("?", " ?")
        for token in line.split():
            yield token.lower()

def parse_list_aux(tokenstream):
    # Leading "(" has already been swallowed.
    while True:
        try:
            token = next(tokenstream)
        except StopIteration:
            raise ParseError("Missing ')'")
        if token == ")":
            return
        elif token == "(":
            yield list(parse_list_aux(tokenstream))
        else:
            yield token
