from pathlib import Path
import argparse
import sys

options = None

# The following code is used to show the usage instructions from argparse as
# "usage: python3 -m translate" (or analogously for other python executables)
# instead of "usage: __main__.py" if the package is called as a package.
# This is the default behaviour of argparse.ArgumentParser since Python 3.12,
# so we can remove this code once that is the oldest supported python version.
def infer_prog():
    main_mod = sys.modules['__main__']
    spec = getattr(main_mod, '__spec__', None)

    if spec is not None:
        # Invoked via `python -m ...`
        module_name = spec.name
        if module_name.endswith('.__main__'):
            module_name = module_name.rsplit('.', 1)[0]
        python_exec = Path(sys.executable).name
        return f"{python_exec} -m {module_name}"
    else:
        # Invoked as script directly.
        return Path(sys.argv[0]).name


def parse_args(args=None):
    argparser = argparse.ArgumentParser(prog=infer_prog())
    argparser.add_argument(
        "domain", help="path to domain pddl file")
    argparser.add_argument(
        "task", help="path to task pddl file")
    argparser.add_argument(
        "--relaxed", dest="generate_relaxed_task", action="store_true",
        help="output relaxed task (no delete effects)")
    argparser.add_argument(
        "--full-encoding",
        dest="use_partial_encoding", action="store_false",
        help="By default we represent facts that occur in multiple "
        "mutex groups only in one variable. Using this parameter adds "
        "these facts to multiple variables. This can make the meaning "
        "of the variables clearer, but increases the number of facts.")
    argparser.add_argument(
        "--invariant-generation-max-candidates", default=100000, type=int,
        help="max number of candidates for invariant generation "
        "(default: %(default)d). Set to 0 to disable invariant "
        "generation and obtain only binary variables. The limit is "
        "needed for grounded input files that would otherwise produce "
        "too many candidates.")
    argparser.add_argument(
        "--sas-file", default="output.sas",
        help="path to the SAS output file (default: %(default)s)")
    argparser.add_argument(
        "--invariant-generation-max-time", default=300, type=int,
        help="max time for invariant generation (default: %(default)ds)")
    argparser.add_argument(
        "--add-implied-preconditions", action="store_true",
        help="infer additional preconditions. This setting can cause a "
        "severe performance penalty due to weaker relevance analysis "
        "(see issue7).")
    argparser.add_argument(
        "--keep-unreachable-facts",
        dest="filter_unreachable_facts", action="store_false",
        help="keep facts that can't be reached from the initial state")
    argparser.add_argument(
        "--skip-variable-reordering",
        dest="reorder_variables", action="store_false",
        help="do not reorder variables based on the causal graph. Do not use "
        "this option with the causal graph heuristic!")
    argparser.add_argument(
        "--keep-unimportant-variables",
        dest="filter_unimportant_vars", action="store_false",
        help="keep variables that do not influence the goal in the causal graph")
    argparser.add_argument(
        "--keep-no-ops", action="store_true",
        help="keep operators without effects in the output")
    argparser.add_argument(
        "--dump-task", action="store_true",
        help="dump human-readable SAS+ representation of the task")
    argparser.add_argument(
        "--layer-strategy", default="min", choices=["min", "max"],
        help="How to assign layers to derived variables. 'min' attempts to put as "
        "many variables into the same layer as possible, while 'max' puts each variable "
        "into its own layer unless it is part of a cycle.")
    return argparser.parse_args(args)


def get_options():
    if options is None:
        msg = ("No options provided (via options.set_options(...)). For example"
               " 'options.set_options([<domain file>, <problem_file>])'.")
        raise RuntimeError(msg)
    return options


def set_options(arguments=None):
    global options
    options = parse_args(arguments)
