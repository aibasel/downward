# -*- coding: utf-8 -*-

import argparse

from . import aliases


DESCRIPTION = """Fast Downward driver script. Arguments that do not
have a special meaning for the driver (see below) are passed on to the
search component."""


def _split_off_filenames(planner_args):
    """Given the list of arguments to be passed on to the planner
    components, split it into a prefix of filenames and a suffix of
    options. Returns a pair (filenames, options).

    If a "--" separator is present, the last such separator serves as
    the border between filenames and options. The separator itself is
    not returned. (This implies that "--" can be a filename, but never
    an option to a planner component.)

    If no such separator is present, the first argument that begins
    with "-" and consists of at least two characters starts the list
    of options, and all previous arguments are filenames."""

    if "--" in planner_args:
        separator_pos = planner_args.rindex("--")
        num_filenames = separator_pos
        del planner_args[separator_pos]
    else:
        num_filenames = 0
        for arg in planner_args:
            # We treat "-" by itself as a filename because by common
            # convention it denotes stdin or stdout, and we might want
            # to support this later.
            if arg.startswith("-") and arg != "-":
                break
            num_filenames += 1
    return planner_args[:num_filenames], planner_args[num_filenames:]


def _split_planner_args(parser, args):
    """Partition args.planner_args, the list of arguments for the
    planner components, into args.filenames, args.translate_options,
    arge.preprocess_options and args.search_options. Modifies args
    directly and removes the original args.planner_args list."""

    args.filenames, options = _split_off_filenames(args.planner_args)

    args.translate_options = []
    args.preprocess_options = []
    args.search_options = []

    curr_options = args.search_options
    for option in options:
        if option == "--translate-options":
            curr_options = args.translate_options
        elif option == "--preprocess-options":
            curr_options = args.preprocess_options
        elif option == "--search-options":
            curr_options = args.search_options
        else:
            curr_options.append(option)


def _check_mutex_args(parser, args, required=False):
    for pos, (name1, is_specified1) in enumerate(args):
        for name2, is_specified2 in args[pos + 1:]:
            if is_specified1 and is_specified2:
                parser.error("cannot combine %s with %s" % (name1, name2))
    if required and not any(is_specified for _, is_specified in args):
        parser.error("exactly one of {%s} has to be specified" %
            ", ".join(name for name, _ in args))


def _looks_like_search_input(filename):
    # We don't currently have a good way to distinguish preprocess and
    # search inputs without going through most of the file, so we
    # don't even try.
    with open(filename) as input_file:
        first_line = next(input_file, "").rstrip()
    return first_line == "begin_version"


def _set_components_automatically(parser, args):
    """Guess which planner components to run based on the specified
    filenames and set args.components accordingly. Currently
    implements some simple heuristics:

    1. If there is exactly one input file and it looks like a
       Fast-Downward-generated file, run search only.
    2. Otherwise, run all components."""

    if len(args.filenames) == 1 and _looks_like_search_input(args.filenames[0]):
        args.components = ["search"]
    else:
        args.components = ["translate", "preprocess", "search"]


def _set_components_and_inputs(parser, args):
    """Set args.components to the planner components to be run
    and set args.translate_inputs, args.preprocess_input and
    args.search_input to the correct input filenames.

    Rules:
    1. If any --run-xxx option is specified, then the union
       of the specified components is run.
    2. It is an error to specify running the translator and
       search, but not the preprocessor.
    3. If nothing is specified, use automatic rules. See
       separate function."""

    args.components = []
    if args.run_translator or args.run_all:
        args.components.append("translate")
    if args.run_preprocessor or args.run_all:
        args.components.append("preprocess")
    if args.run_search or args.run_all:
        args.components.append("search")

    if args.components == ["translate", "search"]:
        parser.error("cannot run translator and search without preprocessor")

    if not args.components:
        _set_components_automatically(parser, args)

    args.translate_inputs = []
    args.preprocess_input = "output.sas"
    args.search_input = "output"

    assert args.components
    first = args.components[0]
    if first == "translate":
        if len(args.filenames) not in [1, 2]:
            parser.error("translator needs one or two input files")
        args.translate_inputs = args.filenames
    elif first == "preprocess":
        if len(args.filenames) != 1:
            parser.error("preprocessor needs exactly one input file")
        args.preprocess_input, = args.filenames
    elif first == "search":
        if len(args.filenames) != 1:
            parser.error("search needs exactly one input file")
        args.search_input, = args.filenames
    else:
        assert False, first


def parse_args():
    # TODO: Need better usage string. We might also want to improve
    # the help output more generally. Note that there are various ways
    # to finetune this by including a formatter, an epilog, etc.
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument(
        "--alias",
        help="run a config with an alias (e.g. seq-sat-lama-2011)")
    parser.add_argument(
        "--debug", action="store_true",
        help="use debug mode for search component")
    parser.add_argument(
        "--ipc", dest="alias",
        help="same as --alias")
    parser.add_argument(
        "--plan-file", metavar="FILE", default="sas_plan",
        help="write plan(s) to FILE{.1,.2,...} (default: %(default)s)")
    parser.add_argument(
        "--portfolio", metavar="FILE",
        help="run a portfolio specified in FILE")
    parser.add_argument(
        "--run-all", action="store_true",
        help="run all components of the planner")
    parser.add_argument(
        "--run-translator", action="store_true",
        help="run translator component of the planner")
    parser.add_argument(
        "--run-preprocessor", action="store_true",
        help="run preprocessor component of the planner")
    parser.add_argument(
        "--run-search", action="store_true",
        help="run search component of the planner")
    parser.add_argument(
        "--show-aliases", action="store_true",
        help="show the known aliases; don't run search")
    parser.add_argument(
        "planner_args", nargs=argparse.REMAINDER,
        help="file names and options passed on to planner components")

    # Using argparse.REMAINDER relies on the fact that the first
    # argument that doesn't belong to the driver doesn't look like an
    # option, i.e., doesn't start with "-". This is usually satisfied
    # because the argument is a filename; in exceptional cases, "--"
    # can be used as an explicit separator. For example, "./plan.py --
    # --help" passes "--help" to the search code.

    args = parser.parse_args()

    _split_planner_args(parser, args)

    _check_mutex_args(parser, [
            ("--alias", args.alias is not None),
            ("--portfolio", args.portfolio is not None),
            ("options for search component", bool(args.search_options))],
        required=True)

    if args.alias:
        try:
            aliases.set_options_for_alias(args.alias, args)
        except KeyError:
            parser.error("unknown alias: %r" % args.alias)

    _set_components_and_inputs(parser, args)

    return args
