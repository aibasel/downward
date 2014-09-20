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
        # TODO: Preserve exit code.
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
        call_cmd(executable, args.search_options, stdin=args.search_input)
    print "***"
