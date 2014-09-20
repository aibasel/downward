# -*- coding: utf-8 -*-

import portfolio

import os
import os.path
import subprocess
import sys
import types


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
    print "*** translator inputs: %s" % args.translate_inputs
    print "*** translator arguments: %s" % args.translate_options
    call_cmd(TRANSLATE, args.translate_inputs + args.translate_options)
    print "***"


def run_preprocess(args):
    print "*** Running preprocessor."
    print "*** preprocessor arguments: %s" % args.preprocess_options
    call_cmd(PREPROCESS, args.preprocess_options, stdin=args.preprocess_input)
    print "***"


def run_search(args):
    print "*** Running search."

    if args.debug:
        executable = os.path.join(SEARCH_DIR, "downward-debug")
    else:
        executable = os.path.join(SEARCH_DIR, "downward-release")
    print "*** executable:", executable

    if args.portfolio:
        assert not args.search_options
        # TODO: Implement portfolios. Don't forget exit code. Take
        # into account the time used by *this* process too for the
        # portfolio time slices. After all, we used to take into
        # account time for dispatch script and unitcost script, and
        # these are now in-process.
        environment = {}
        def mock_run(configs, optimal=True, final_config=None,
                     final_config_builder=None, timeout=None):
            portfolio.run(executable, args.search_input, configs, optimal,
                          final_config, final_config_builder, timeout)

        mock_portfolio_module = types.ModuleType("portfolio", "Mock portfolio module")
        sys.modules["portfolio"] = mock_portfolio_module
        mock_portfolio_module.__dict__['run'] = mock_run
        execfile(args.portfolio, environment)
    else:
        print "*** final search options:", args.search_options
        write_elapsed_time()
        call_cmd(executable, args.search_options, stdin=args.search_input)
    print "***"
