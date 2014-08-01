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
    """Partitions args.planner_args, the list of arguments for the
    planner components, into args.filenames, args.translate_options,
    arge.preprocess_options and args.search_options. Modifies args
    directly and removes the original args.planner_args list."""

    args.filenames, options = _split_off_filenames(args.planner_args)
    del args.planner_args

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


def _check_arg_conflicts(parser, possible_conflicts):
    for pos, (name1, is_specified1) in enumerate(possible_conflicts):
        for name2, is_specified2 in possible_conflicts[pos + 1:]:
            if is_specified1 and is_specified2:
                parser.error("cannot combine %s with %s" % (name1, name2))


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
        help="run search component in debug mode")
    parser.add_argument(
        "--ipc", dest="alias",
        help="same as --alias")
    parser.add_argument(
        "--portfolio", metavar="FILE",
        help="run a portfolio specified in FILE")
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
    args.unit_cost_search_options = None

    _check_arg_conflicts(parser, [
            ("--alias", args.alias is not None),
            ("--portfolio", args.portfolio is not None),
            ("options for search component", bool(args.search_options))])

    if args.alias:
        try:
            aliases.set_options_for_alias(args.alias, args)
        except KeyError:
            parser.error("unknown alias: %r" % args.alias)

    return args
