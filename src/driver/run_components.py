# -*- coding: utf-8 -*-

import os
import os.path
import subprocess
import sys

import portfolio


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
        portfolio.run(args.portfolio, executable, args.search_input)
    else:
        print "*** final search options:", args.search_options
        call_cmd(executable, args.search_options, stdin=args.search_input)
    print "***"
