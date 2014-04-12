#! /usr/bin/env python

import argparse
import os
import os.path
import resource
import sys

import aliases
import input_analyzer


BASE_DIR = os.path.abspath(os.path.dirname(__file__))

DESCRIPTION = """Wrapper for the Fast Downward search component.
Arguments that do not have a special meaning for the wrapper (see below)
are passed on to the actual search component."""


def set_memory_limit():
    # We don't want Python to gobble up too much memory in case we're
    # running a portfolio, so we set a 50 MB memory limit for
    # ourselves. We set a "soft" limit, which limits ourselves, but
    # not a "hard" limit, which would prevent us from raising the
    # limit for our child processes. If an existing soft limit is
    # already lower, we leave it unchanged.

    # On the maia cluster, 20 MB have been tested to be sufficient; 18
    # MB were insufficent. With 50 MB, we should have a bit of a
    # safety net.

    ALLOWED_MEMORY = 50 * 1024 * 1024
    soft, hard = resource.getrlimit(resource.RLIMIT_AS)
    if soft == resource.RLIM_INFINITY or soft > ALLOWED_MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (ALLOWED_MEMORY, hard))


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
    parser.add_argument("--file", default="output",
                        help="preprocessed input file (default: %(default)s)")
    parser.add_argument("--help-search", action="store_true",
                        help="pass --help argument to search component")
    parser.add_argument("--ipc", dest="alias",
                        help="same as --alias")
    parser.add_argument("--portfolio", metavar="FILE",
                        help="run a portfolio specified in FILE")
    parser.add_argument("--show-aliases", action="store_true",
                        help="show the known aliases; don't run search")

    args, search_args = parser.parse_known_args()
    args.search_args = search_args
    args.unit_cost_search_args = None

    print "*** raw args:", args, search_args

    check_arg_conflicts(parser, [
            ("--alias", args.alias is not None),
            ("--help-search", args.help_search),
            ("--portfolio", args.portfolio is not None),
            ("arguments to search component", bool(search_args))])

    if args.help_search:
        args.search_args = ["--help"]

    if args.alias:
        try:
            aliases.set_args_for_alias(args.alias, args)
        except KeyError:
            parser.error("unknown alias: %r" % args.alias)

    return args


def main():
    args = parse_args()

    print "*** processed args:", args

    if args.portfolio is not None:
        set_memory_limit()

    if args.show_aliases:
        aliases.show_aliases()
        sys.exit()

    if args.unit_cost_search_args is not None:
        is_unit_cost = input_analyzer.is_unit_cost(args.file)
        if is_unit_cost:
            args.search_args = args.unit_cost_search_args
            print "using special configuration for unit-cost problems"

    if args.debug:
        executable = os.path.join(BASE_DIR, "downward-debug")
    else:
        executable = os.path.join(BASE_DIR, "downward-release")
    print "*** executable:", executable

    if args.portfolio:
        assert not args.search_args
        # TODO: Implement portfolios. Don't forget exit code. Take
        # into account the time used by *this* process too for the
        # portfolio time slices. After all, we used to take into
        # account time for dispatch script and unitcost script, and
        # these are now in-process.
        raise NotImplementedError
    else:
        print "*** final search args:", args.search_args
        if not args.help_search:
            # Redirect stdin to the input file.
            os.close(sys.stdin.fileno())
            os.open(args.file, os.O_RDONLY)
        # Run planner with exec to conserve memory.
        # Note: Flushing is necessary; otherwise the output can get lots
        # when Python is not running in line-buffered mode (e.g. when
        # redirecting output to a file or pipe).
        sys.stdout.flush()
        sys.stderr.flush()
        os.execv(executable, [executable] + args.search_args)


if __name__ == "__main__":
    # TODO: Test suite that tests all the aliases, including
    # unit/general case. Ditto for the portfolios.
    main()
