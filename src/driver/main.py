# -*- coding: utf-8 -*-

import argparse
import os
import os.path
import resource
import subprocess
import sys

from . import aliases
from . import input_analyzer


DRIVER_DIR = os.path.abspath(os.path.dirname(__file__))
PARENT_DIR = os.path.dirname(DRIVER_DIR)
TRANSLATE = os.path.join(PARENT_DIR, "translate", "translate.py")
PREPROCESS = os.path.join(PARENT_DIR, "preprocess", "preprocess")
SEARCH_DIR = os.path.join(PARENT_DIR, "search")


DESCRIPTION = """Fast Downward driver script. Arguments that do not
have a special meaning for the driver (see below) are passed on to the
search component."""


def write_elapsed_time():
    ## Note: According to the os.times documentation, Windows sets the
    ## child time components to 0, so this won't work properly under
    ## Windows.
    ##
    ## TODO: Find a solution for this. A simple solution might be to
    ## just document this as a limitation under Windows that causes
    ## time slices for portfolios to be allocated slightly wrongly.
    ## Another solution would be to base time slices on wall-clock
    ## time under Windows.
    times = os.times()
    child_elapsed = times[2] + times[3]
    with open("elapsed.time", "w") as time_file:
        print >> time_file, child_elapsed


def call_cmd(cmd, stdin=None):
    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            subprocess.check_call(cmd, stdin=stdin_file)
    else:
        subprocess.check_call(cmd)


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
    parser.add_argument("--input-file-search", default="output",
                        help="preprocessed input file (default: %(default)s)")
    parser.add_argument("--help-search", action="store_true",
                        help="pass --help argument to search component")
    parser.add_argument("--ipc", dest="alias",
                        help="same as --alias")
    parser.add_argument("--portfolio", metavar="FILE",
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
            ("--portfolio", args.portfolio is not None),
            ("arguments to search component", bool(args.search_args))])

    if args.help_search:
        args.search_args = ["--help"]

    if args.alias:
        try:
            aliases.set_args_for_alias(args.alias, args)
        except KeyError:
            parser.error("unknown alias: %r" % args.alias)

    return args


def run_translate(args):
    print "*** Running translator."
    print "*** translator arguments: %s" % args.translate_args
    call_cmd([TRANSLATE] + args.translate_args)
    print "***"


def run_preprocess(args):
    print "*** Running preprocessor."
    print "*** preprocessor arguments: %s" % args.preprocess_args
    call_cmd([PREPROCESS] + args.preprocess_args, stdin="output.sas")
    print "***"


def run_search(args):
    print "*** Running search."

    if args.unit_cost_search_args is not None:
        if input_analyzer.is_unit_cost(args.input_file_search):
            print "*** using special configuration for unit-cost problems"
            args.search_args = args.unit_cost_search_args

    if args.debug:
        executable = os.path.join(SEARCH_DIR, "downward-debug")
    else:
        executable = os.path.join(SEARCH_DIR, "downward-release")
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
        write_elapsed_time()
        if args.help_search:
            call_cmd([executable] + list(args.search_args))
        else:
            call_cmd([executable] + list(args.search_args), stdin=args.input_file_search)
    print "***"


def main():
    args = parse_args()
    print "*** processed args:", args

    if args.portfolio is not None:
        set_memory_limit()

    if args.show_aliases:
        aliases.show_aliases()
        sys.exit()

    if len(args.translate_args) == 0:
        # This is of course a HACK.
        print "*** no positional filenames: run search only"
        run_search(args)
    else:
        run_translate(args)
        run_preprocess(args)
        run_search(args)

    ## TODO: The old "plan" script preserved the exit code of the
    ## search component. Make sure that this one does, too, both in
    ## the case where only search is run and in the case where the
    ## whole planner is run. It is probably better to wait with this
    ## until the functionality of the driver has progressed further,
    ## though.


if __name__ == "__main__":
    # TODO: Test suite that tests all the aliases, including
    # unit/general case. Ditto for the portfolios.
    main()
