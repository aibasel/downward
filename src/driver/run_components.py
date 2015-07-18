# -*- coding: utf-8 -*-

import logging
import os.path
import sys

from . import call
from . import limits
from . import portfolio_runner
from .plan_manager import PlanManager
from .util import SRC_DIR


TRANSLATE = os.path.join(SRC_DIR, "translate", "translate.py")
PREPROCESS = os.path.join(SRC_DIR, "preprocess", "preprocess")
SEARCH_DIR = os.path.join(SRC_DIR, "search")


def check_for_executable(executable, debug):
    if not os.path.exists(executable):
        target = " debug" if debug else ""
        raise IOError(
            "Could not find {executable}. "
            "Please run './build_all{target}'.".format(**locals()))


def print_component_settings(nick, inputs, options, time_limit, memory):
    logging.info("{} input: {}".format(nick, inputs))
    logging.info("{} arguments: {}".format(nick, options))
    if time_limit is not None:
        time_limit = str(time_limit) + "s"
    logging.info("{} time_limit: {}".format(nick, time_limit))
    if memory is not None:
        memory = "{:.2} MB".format(limits.convert_to_mb(memory))
    logging.info("{} memory limit: {}".format(nick, memory))


def call_component(executable, options, stdin=None,
                   time_limit=None, memory_limit=None):
    if executable.endswith(".py"):
        options.insert(0, executable)
        executable = sys.executable
    call.check_call(
        [executable] + options,
        stdin=stdin, time_limit=time_limit, memory_limit=memory_limit)


def run_translate(args):
    logging.info("Running translator.")
    time_limit = limits.get_time_limit(
        args.translate_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.translate_memory_limit, args.overall_memory_limit)
    print_component_settings(
        "translator", args.translate_inputs, args.translate_options,
        time_limit, memory_limit)
    call_component(
        TRANSLATE, args.translate_inputs + args.translate_options,
        time_limit=time_limit, memory_limit=memory_limit)


def run_preprocess(args):
    logging.info("Running preprocessor.")
    time_limit = limits.get_time_limit(
        args.preprocess_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.preprocess_memory_limit, args.overall_memory_limit)
    print_component_settings(
        "preprocessor", args.preprocess_input, args.preprocess_options,
        time_limit, memory_limit)
    check_for_executable(PREPROCESS, args.debug)
    call_component(
        PREPROCESS, args.preprocess_options,
        stdin=args.preprocess_input,
        time_limit=time_limit, memory_limit=memory_limit)


def run_search(args):
    logging.info("Running search.")
    time_limit = limits.get_time_limit(
        args.search_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.search_memory_limit, args.overall_memory_limit)
    print_component_settings(
        "search", args.search_input, args.search_options,
        time_limit, memory_limit)

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
            time_limit, memory_limit)
    else:
        if not args.search_options:
            raise ValueError(
                "search needs --alias, --portfolio, or search options")
        if "--help" not in args.search_options:
            args.search_options.extend(["--internal-plan-file", args.plan_file])
        call_component(
            executable, args.search_options,
            stdin=args.search_input,
            time_limit=time_limit, memory_limit=memory_limit)
