# -*- coding: utf-8 -*-

import os
import os.path
import subprocess
import sys

from . import input_analyzer


DRIVER_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.dirname(DRIVER_DIR)
TRANSLATE = os.path.join(SRC_DIR, "translate", "translate.py")
PREPROCESS = os.path.join(SRC_DIR, "preprocess", "preprocess")
SEARCH_DIR = os.path.join(SRC_DIR, "search")


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

    if args.debug:
        executable = os.path.join(SEARCH_DIR, "downward-debug")
    else:
        executable = os.path.join(SEARCH_DIR, "downward-release")
    print "*** executable:", executable

    if args.help_search:
        call_cmd(executable, args.search_args)
    elif args.portfolio_file:
        assert not args.search_args
        # TODO: Implement portfolios. Don't forget exit code. Take
        # into account the time used by *this* process too for the
        # portfolio time slices. After all, we used to take into
        # account time for dispatch script and unitcost script, and
        # these are now in-process.

        # TODO: Should the portfolios care about the
        # unit-cost/general-cost disinction, too?
        raise NotImplementedError
    else:
        INPUT_FILE = "output"

        search_args = args.search_args

        if args.unit_cost_search_args is not None:
            if input_analyzer.is_unit_cost(INPUT_FILE):
                print "*** using special configuration for unit-cost problems"
                search_args = args.unit_cost_search_args

        print "*** final search args:", search_args
        write_elapsed_time()
        call_cmd(executable, search_args, stdin=INPUT_FILE)
    print "***"
