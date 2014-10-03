# -*- coding: utf-8 -*-

import logging
import os
import os.path
import subprocess
import sys

from . import portfolio_runner


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
    logging.info("Running translator.")
    logging.info("translator inputs: %s" % args.translate_inputs)
    logging.info("translator arguments: %s" % args.translate_options)
    call_cmd(TRANSLATE, args.translate_inputs + args.translate_options)


def run_preprocess(args):
    logging.info("Running preprocessor.")
    logging.info("preprocessor arguments: %s" % args.preprocess_options)
    call_cmd(PREPROCESS, args.preprocess_options, stdin=args.preprocess_input)


def run_search(args):
    logging.info("Running search.")

    if args.debug:
        executable = os.path.join(SEARCH_DIR, "downward-debug")
    else:
        executable = os.path.join(SEARCH_DIR, "downward-release")
    logging.info("executable: %s" % executable)

    if args.portfolio:
        assert not args.search_options
        portfolio_runner.run(
            args.portfolio, executable, args.search_input, args.plan_file)
    else:
        if not args.search_options:
            raise SystemExit(
                "search needs --alias, --portfolio, or search options")
        if "--plan-file" not in args.search_options:
            args.search_options.extend(["--plan-file", args.plan_file])
        logging.info("final search options: %s" % args.search_options)
        call_cmd(executable, args.search_options, stdin=args.search_input)
