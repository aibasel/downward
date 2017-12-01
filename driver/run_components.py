# -*- coding: utf-8 -*-

import errno
import logging
import os.path
import subprocess
import sys

from . import call
from . import limits
from . import portfolio_runner
from . import returncodes
from . import util
from .plan_manager import PlanManager

VALIDATE_MEMORY_LIMIT_IN_B = 3221225472 # 3 GB
#TODO: We might want to turn translate into a module and call it with "python -m translate".
REL_TRANSLATE_PATH = os.path.join("translate", "translate.py")
if os.name == "posix":
    REL_SEARCH_PATH = "downward"
    VALIDATE = "validate"
elif os.name == "nt":
    REL_SEARCH_PATH = "downward.exe"
    VALIDATE = "validate.exe"
else:
    sys.exit("Unsupported OS: " + os.name)

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

def print_component_settings(nick, executable, inputs, options, time_limit, memory_limit):
    logging.info("{} executable: {}".format(nick, executable))
    logging.info("{} input: {}".format(nick, inputs))
    logging.info("{} arguments: {}".format(nick, options))
    if time_limit is not None:
        time_limit = str(time_limit) + "s"
    logging.info("{} time limit: {}".format(nick, time_limit))
    if memory_limit is not None:
        memory_limit = int(limits.convert_to_mb(memory_limit))
        memory_limit = str(memory_limit) + " MB"
    logging.info("{} memory limit: {}".format(nick, memory_limit))


def print_callstring(executable, options, stdin=None):
    parts = [executable] + options
    parts = [util.shell_escape(x) for x in parts]
    if stdin is not None:
        parts.extend(["<", util.shell_escape(stdin)])
    logging.info("callstring: %s" % " ".join(parts))


def run_translate(args):
    logging.info("Running translator.")
    time_limit = limits.get_time_limit(
        args.translate_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.translate_memory_limit, args.overall_memory_limit)
    translate = get_executable(args.build, REL_TRANSLATE_PATH)
    print_component_settings(
        "translator", translate, args.translate_inputs, args.translate_options,
        time_limit, memory_limit)

    options = [translate] + args.translate_inputs + args.translate_options
    executable = sys.executable
    assert executable, "Path to interpreter could not be found"
    print_callstring(executable, options)

    try:
        call.check_call(
            [executable] + options,
            time_limit=time_limit,
            memory_limit=memory_limit)
    except subprocess.CalledProcessError as err:
        if err.returncode == 1:
            return (returncodes.TRANSLATE_CRITICAL_ERROR, False)

        assert err.returncode >= 10
        return (err.returncode, False)
    else:
        return (0, True)


def run_search(args):
    logging.info("Running search (%s)." % args.build)
    time_limit = limits.get_time_limit(
        args.search_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.search_memory_limit, args.overall_memory_limit)
    executable = get_executable(args.build, REL_SEARCH_PATH)
    print_component_settings(
        "search", executable, args.search_input, args.search_options,
        time_limit, memory_limit)

    plan_manager = PlanManager(args.plan_file)
    plan_manager.delete_existing_plans()

    if args.portfolio:
        assert not args.search_options
        logging.info("search portfolio: %s" % args.portfolio)
        return portfolio_runner.run(
            args.portfolio, executable, args.search_input, plan_manager,
            time_limit, memory_limit)
    else:
        if not args.search_options:
            raise ValueError(
                "search needs --alias, --portfolio, or search options")
        if "--help" not in args.search_options:
            args.search_options.extend(["--internal-plan-file", args.plan_file])

        print_callstring(executable, args.search_options, args.search_input)

        try:
            call.check_call(
                [executable] + args.search_options,
                stdin=args.search_input,
                time_limit=time_limit,
                memory_limit=memory_limit)
        except subprocess.CalledProcessError as err:
            # TODO: if we ever add support for SEARCH_PLAN_FOUND_AND_* directly
            # in the planner, this assertion no longer holds. Furthermore, we
            # would need to return (err.returncode, True) if the returncode is
            # in [0..10].
            assert err.returncode >= 10
            return (err.returncode, False)
        else:
            return (0, True)


def run_validate(args):
    logging.info("Running validate.")

    num_files = len(args.filenames)
    if num_files == 1:
        task, = args.filenames
        domain = util.find_domain_filename(task)
    elif num_files == 2:
        domain, task = args.filenames
    else:
        raise ValueError("validate needs one or two PDDL input files.")

    executable = VALIDATE
    plan_files = list(PlanManager(args.plan_file).get_existing_plans())
    validate_inputs = [domain, task] + plan_files

    print_component_settings(
        "validate", executable, validate_inputs, [],
        time_limit=None, memory_limit=VALIDATE_MEMORY_LIMIT_IN_B)
    print_callstring(executable, validate_inputs)

    try:
        call.check_call(
            [executable] + validate_inputs,
            memory_limit=VALIDATE_MEMORY_LIMIT_IN_B)
    except OSError as err:
        if err.errno == errno.ENOENT:
            sys.exit("Error: {} not found. Is it on the PATH?".format(executable))
        else:
            raise
    else:
        return (0, True)
