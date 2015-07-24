# -*- coding: utf-8 -*-

import logging
import os.path
import sys

from . import call
from . import limits
from . import portfolio_runner
from .plan_manager import PlanManager
from .util import BUILDS_DIR

#TODO: We might want to turn translate into a module and call it with "python -m translate".
REL_TRANSLATE_PATH = os.path.join("bin", "translate", "translate.py")
REL_PREPROCESS_PATH = os.path.join("bin", "preprocess")
REL_SEARCH_PATH = os.path.join("bin", "downward")

def get_executable(build, rel_path):
    build_dir = os.path.join(BUILDS_DIR, build)
    if not os.path.exists(build_dir):
        raise IOError(
            "Could not find build '{build}' at {build_dir}. "
            "Please run './build.py {build}'.".format(**locals()))

    abs_path = os.path.join(build_dir, rel_path)
    if not os.path.exists(abs_path):
        raise IOError(
            "Could not find '{rel_path}' in build '{build}'. "
            "Please run './build.py {build}'.".format(**locals()))

    return abs_path

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
    translate = get_executable(args.build, REL_TRANSLATE_PATH)
    call_component(
        translate, args.translate_inputs + args.translate_options,
        time_limit=time_limit, memory_limit=memory_limit)


def run_preprocess(args):
    logging.info("Running preprocessor (%s)." % args.build)
    time_limit = limits.get_time_limit(
        args.preprocess_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.preprocess_memory_limit, args.overall_memory_limit)
    print_component_settings(
        "preprocessor", args.preprocess_input, args.preprocess_options,
        time_limit, memory_limit)
    preprocess = get_executable(args.build, REL_PREPROCESS_PATH)
    call_component(
        preprocess, args.preprocess_options,
        stdin=args.preprocess_input,
        time_limit=time_limit, memory_limit=memory_limit)


def run_search(args):
    logging.info("Running search (%s)." % args.build)
    time_limit = limits.get_time_limit(
        args.search_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.search_memory_limit, args.overall_memory_limit)
    print_component_settings(
        "search", args.search_input, args.search_options,
        time_limit, memory_limit)

    plan_manager = PlanManager(args.plan_file)
    plan_manager.delete_existing_plans()

    search = get_executable(args.build, REL_SEARCH_PATH)
    logging.info("search executable: %s" % search)

    if args.portfolio:
        assert not args.search_options
        logging.info("search portfolio: %s" % args.portfolio)
        portfolio_runner.run(
            args.portfolio, search, args.search_input, plan_manager,
            time_limit, memory_limit)
    else:
        if not args.search_options:
            raise ValueError(
                "search needs --alias, --portfolio, or search options")
        if "--help" not in args.search_options:
            args.search_options.extend(["--internal-plan-file", args.plan_file])
        call_component(
            search, args.search_options,
            stdin=args.search_input,
            time_limit=time_limit, memory_limit=memory_limit)
