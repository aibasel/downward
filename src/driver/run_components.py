# -*- coding: utf-8 -*-

import logging
import os.path
import sys

from . import call
from . import limits
from . import portfolio_runner
from .plan_manager import PlanManager


DRIVER_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.dirname(DRIVER_DIR)
TRANSLATE = os.path.join(SRC_DIR, "translate", "translate.py")
PREPROCESS = os.path.join(SRC_DIR, "preprocess", "preprocess")
SEARCH_DIR = os.path.join(SRC_DIR, "search")


def check_for_executable(executable, debug):
    if not os.path.exists(executable):
        target = " debug" if debug else ""
        raise IOError(
            "Could not find {executable}. "
            "Please run './build_all{target}'.".format(**locals()))


def print_component_settings(nick, inputs, options, timeout, memory):
    logging.info("{} input: {}".format(nick, inputs))
    logging.info("{} arguments: {}".format(nick, options))
    if timeout is not None:
        timeout = str(timeout) + "s"
    logging.info("{} timeout: {}".format(nick, timeout))
    logging.info("{} memory limit: {:.2} MB".format(
        nick, limits.convert_to_mb(memory)))


def call_component(executable, options, stdin=None, timeout=None, memory=None):
    if executable.endswith(".py"):
        options.insert(0, executable)
        executable = sys.executable
    call.check_call(
        [executable] + options,
        stdin=stdin, timeout=timeout, memory=memory)


def run_translate(args):
    logging.info("Running translator.")
    timeout = limits.get_timeout(args.translate_timeout, args.overall_timeout)
    memory = limits.get_memory_limit(args.translate_memory, args.overall_memory)
    print_component_settings(
        "translator", args.translate_inputs, args.translate_options,
        timeout, memory)
    call_component(
        TRANSLATE, args.translate_inputs + args.translate_options,
        timeout=timeout, memory=memory)


def run_preprocess(args):
    logging.info("Running preprocessor.")
    timeout = limits.get_timeout(args.preprocess_timeout, args.overall_timeout)
    memory = limits.get_memory_limit(args.preprocess_memory, args.overall_memory)
    print_component_settings(
        "preprocessor", args.preprocess_input, args.preprocess_options,
        timeout, memory)
    check_for_executable(PREPROCESS, args.debug)
    call_component(
        PREPROCESS, args.preprocess_options,
        stdin=args.preprocess_input, timeout=timeout, memory=memory)


def run_search(args):
    logging.info("Running search.")
    timeout = limits.get_timeout(args.search_timeout, args.overall_timeout)
    memory = limits.get_memory_limit(args.search_memory, args.overall_memory)
    print_component_settings(
        "search", args.search_input, args.search_options, timeout, memory)

    plan_manager = PlanManager(args.plan_file)
    plan_manager.delete_existing_plans()

    if args.debug:
        executable = os.path.join(SEARCH_DIR, "downward-debug")
    else:
        executable = os.path.join(SEARCH_DIR, "downward-release")
    logging.info("search executable: %s" % executable)
    check_for_executable(executable, args.debug)

    if args.portfolio:
        assert not args.search_options
        logging.info("search portfolio: %s" % args.portfolio)
        portfolio_runner.run(
            args.portfolio, executable, args.search_input, plan_manager,
            timeout, memory)
    else:
        if not args.search_options:
            raise ValueError(
                "search needs --alias, --portfolio, or search options")
        if "--help" not in args.search_options:
            args.search_options.extend(["--internal-plan-file", args.plan_file])
        call_component(
            executable, args.search_options,
            stdin=args.search_input, timeout=timeout, memory=memory)
