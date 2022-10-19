from . import lisp_parser
from . import parsing_functions

file_open = open


def parse_pddl_file(type, filename):
    try:
        # The builtin open function is shadowed by this module's open function.
        # We use the Latin-1 encoding (which allows a superset of ASCII, of the
        # Latin-* encodings and of UTF-8) to allow special characters in
        # comments. In all other parts, we later validate that only ASCII is
        # used.
        return lisp_parser.parse_nested_list(file_open(filename,
                                                       encoding='ISO-8859-1'))
    except OSError as e:
        raise SystemExit("Error: Could not read file: %s\nReason: %s." %
                         (e.filename, e))
    except lisp_parser.ParseError as e:
        raise SystemExit("Error: Could not parse %s file: %s\nReason: %s." %
                         (type, filename, e))


def open(domain_filename=None, task_filename=None):
    if domain_filename is None or task_filename is None:
        # Importing options triggers parsing the problem and domain file names
        # as arguments from the command line. We don't import unconditionally
        # at the head of this file because open is exposed in __init__.py
        # and as a result importing anything of the pddl_parser package in
        # external code would then trigger this arg parse.
        import options
        domain_filename = domain_filename or options.domain
        task_filename = task_filename or options.task

    domain_pddl = parse_pddl_file("domain", domain_filename)
    task_pddl = parse_pddl_file("task", task_filename)

    return parsing_functions.parse_task(domain_pddl, task_pddl)
