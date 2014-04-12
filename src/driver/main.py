# -*- coding: utf-8 -*-

import os
import os.path
import resource
import subprocess
import sys

from . import aliases
from . import arguments
from . import input_analyzer


DRIVER_DIR = os.path.abspath(os.path.dirname(__file__))
PARENT_DIR = os.path.dirname(DRIVER_DIR)
TRANSLATE = os.path.join(PARENT_DIR, "translate", "translate.py")
PREPROCESS = os.path.join(PARENT_DIR, "preprocess", "preprocess")
SEARCH_DIR = os.path.join(PARENT_DIR, "search")


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


def call_cmd(cmd, args, stdin=None):
    sys.stdout.flush()
    if stdin:
        with open(stdin) as stdin_file:
            subprocess.check_call([cmd] + args, stdin=stdin_file)
    else:
        subprocess.check_call([cmd] + args)


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


def run_translate(args):
    print "*** Running translator."
    print "*** translator arguments: %s" % args.translate_args
    call_cmd(TRANSLATE, args.translate_args)
    print "***"


def run_preprocess(args):
    print "*** Running preprocessor."
    print "*** preprocessor arguments: %s" % args.preprocess_args
    call_cmd(PREPROCESS, args.preprocess_args, stdin="output.sas")
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
            call_cmd(executable, args.search_args)
        else:
            call_cmd(executable, args.search_args, stdin=args.input_file_search)
    print "***"


def main():
    args = arguments.parse_args()
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
