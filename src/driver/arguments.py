# -*- coding: utf-8 -*-

import argparse
import os.path

from . import aliases


DESCRIPTION = """Fast Downward driver script. Arguments that do not
have a special meaning for the driver (see below) are passed on to the
search component."""


def check_arg_conflicts(parser, possible_conflicts):
    for pos, (name1, is_specified1) in enumerate(possible_conflicts):
        for name2, is_specified2 in possible_conflicts[pos + 1:]:
            if is_specified1 and is_specified2:
                parser.error("cannot combine %s with %s" % (name1, name2))


def parse_args():
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument("--alias",
                        help="run a config with an alias (e.g. seq-sat-lama-2011)")
    parser.add_argument("--debug", action="store_true",
                        help="run search component in debug mode")
    parser.add_argument("--input-file-search", default="output",
                        help="preprocessed input file (default: %(default)s)")
    parser.add_argument("--help-search", action="store_true",
                        help="pass --help argument to search component")
    parser.add_argument("--ipc", dest="alias",
                        help="same as --alias")
    parser.add_argument("--portfolio-file", metavar="FILE",
                        help="run a portfolio specified in FILE")
    parser.add_argument("--show-aliases", action="store_true",
                        help="show the known aliases; don't run search")

    args, unparsed_args = parser.parse_known_args()

    num_filenames = 0
    for arg in unparsed_args:
        if not os.path.exists(arg):
            break
        num_filenames += 1
    if num_filenames >= 3:
        parser.error("too many filenames: %r" % args.filenames)

    args.translate_args = unparsed_args[:num_filenames]
    args.preprocess_args = []
    args.search_args = unparsed_args[num_filenames:]
    args.unit_cost_search_args = None

    check_arg_conflicts(parser, [
            ("--alias", args.alias is not None),
            ("--help-search", args.help_search),
            ("--portfolio-file", args.portfolio_file is not None),
            ("arguments to search component", bool(args.search_args))])

    if args.help_search:
        args.search_args = ["--help"]

    if args.alias:
        try:
            aliases.set_args_for_alias(args.alias, args)
        except KeyError:
            parser.error("unknown alias: %r" % args.alias)

    return args
