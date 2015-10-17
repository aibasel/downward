# -*- coding: utf-8 -*-

import logging
import os.path
import subprocess
import sys

from . import call
from . import exitcodes
from . import limits
from . import portfolio_runner
from . import util
from .plan_manager import PlanManager

#TODO: We might want to turn translate into a module and call it with "python -m translate".
REL_TRANSLATE_PATH = os.path.join("translate", "translate.py")
if os.name == "posix":
    REL_PREPROCESS_PATH = "preprocess"
    REL_SEARCH_PATH = "downward"
    REL_VALIDATE_PATH = "validate"
elif os.name == "nt":
    REL_PREPROCESS_PATH = "preprocess.exe"
    REL_SEARCH_PATH = "downward.exe"
    REL_VALIDATE_PATH = "validate.exe"
else:
    print("Unsupported OS: " + os.name)
    sys.exit(1)

def get_executable(build, rel_path):
    # First, consider 'build' to be a path directly to the binaries.
    # The path can be absolute or relative to the current working
    # directory.
    build_dir = build
    if not os.path.exists(build_dir):
        # If build is not a full path to the binaries, it might be the
        # name of a build in our standard directory structure.
        # in this case, the binaries are in
        #   '<repo-root>/builds/<buildname>/bin'.
        build_dir = os.path.join(util.BUILDS_DIR, build, "bin")
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

def print_component_settings(nick, inputs, options, time_limit, memory_limit):
    logging.info("{} input: {}".format(nick, inputs))
    logging.info("{} arguments: {}".format(nick, options))
    if time_limit is not None:
        time_limit = str(time_limit) + "s"
    logging.info("{} time_limit: {}".format(nick, time_limit))
    if memory_limit is not None:
        memory_limit = "{:.2} MB".format(limits.convert_to_mb(memory_limit))
    logging.info("{} memory limit: {}".format(nick, memory_limit))


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
        try:
            call_component(
                search, args.search_options,
                stdin=args.search_input,
                time_limit=time_limit, memory_limit=memory_limit)
        except subprocess.CalledProcessError as err:
            if err.returncode in exitcodes.EXPECTED_EXITCODES:
                return err.returncode
            else:
                raise
        else:
            return 0


def run_validate(args):
    logging.info("Running validate.")
    plan_files = list(PlanManager(args.plan_file).get_existing_plans())
    num_files = len(args.filenames)
    if "-h" in args.validate_options:
        # Silently swallow input filenames.
        args.validate_inputs = []
    elif num_files == 0:
        raise ValueError("Validation needs one or two PDDL input files.")
    elif num_files == 1:
        task, = args.filenames
        domain = util.find_domain_filename(task)
        args.validate_inputs = [domain, task] + plan_files
    elif num_files == 2:
        if plan_files:
            domain, task = args.filenames
            args.validate_inputs = [domain, task] + plan_files
        else:
            task, solution = args.filenames
            domain = util.find_domain_filename(task)
            args.validate_inputs = [domain, task, solution]
    else:
        domain, task = args.filenames[:2]
        solutions = args.filenames[2:]
        args.validate_inputs = [domain, task] + solutions
    print_component_settings(
        "validate", args.validate_inputs, args.validate_options,
        time_limit=None, memory_limit=None)

    validate = get_executable(args.build, REL_VALIDATE_PATH)
    logging.info("validate executable: %s" % validate)

    call_component(
        validate, args.validate_options + args.validate_inputs)
