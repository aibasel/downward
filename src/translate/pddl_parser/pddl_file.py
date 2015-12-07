try:
    # Python 3.x
    import builtins
except ImportError:
    # Python 2.x
    import __builtin__ as builtins

import options

from . import lisp_parser
from . import parsing_functions


def parse_pddl_file(type, filename):
    try:
        # The builtin open function is shadowed by this module's open function.
        return lisp_parser.parse_nested_list(builtins.open(filename))
    except IOError as e:
        raise SystemExit("Error: Could not read file: %s\nReason: %s." %
                         (e.filename, e))
    except lisp_parser.ParseError as e:
        raise SystemExit("Error: Could not parse %s file: %s\n" % (type, filename))

def open(domain_filename=None, task_filename=None):
    task_filename = task_filename or options.task
    domain_filename = domain_filename or options.domain

    domain_pddl = parse_pddl_file("domain", domain_filename)
    task_pddl = parse_pddl_file("task", task_filename)

    return parsing_functions.parse_task(domain_pddl, task_pddl)
