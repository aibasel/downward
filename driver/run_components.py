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

VALIDATE_MEMORY_LIMIT_IN_B = 3.5 * 1024 * 1024 * 1024
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


def run_translate(args):
    logging.info("Running translator.")
    time_limit = limits.get_time_limit(
        args.translate_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.translate_memory_limit, args.overall_memory_limit)
    translate = get_executable(args.build, REL_TRANSLATE_PATH)
    assert sys.executable, "Path to interpreter could not be found"
    cmd = [sys.executable] + [translate] + args.translate_inputs + args.translate_options

    try:
        stderr, returncode = call.get_error_output_and_returncode(
            "translator",
            cmd,
            time_limit=time_limit,
            memory_limit=memory_limit)

        # We collect stderr of the translator and print it here, unless
        # the translator ran out of memory and all output in stderr is
        # related to MemoryError.
        print_stderr = True
        if returncode == returncodes.TRANSLATE_OUT_OF_MEMORY:
            output_related_to_memory_error = True
            if not stderr:
                output_related_to_memory_error = False
            for line in stderr.splitlines():
                if "MemoryError" not in line:
                    output_related_to_memory_error = False
                    break
            if output_related_to_memory_error:
                print_stderr = False

        if print_stderr and stderr:
            sys.stderr.write(stderr)
            sys.stderr.write("\n")
            sys.stderr.flush()

        if returncode == 0:
            return (0, True)
        elif returncode == 1:
            # Unlikely case that the translator crashed without raising an
            # exception.
            return (returncodes.TRANSLATE_CRITICAL_ERROR, False)
        else:
            # Pass on any other exit code, including in particular signals or
            # exit codes such as running out of memory or time.
            return (returncode, False)
    except Exception as err:
        # Translator crashed
        print(err)
        raise


def run_search(args):
    logging.info("Running search (%s)." % args.build)
    time_limit = limits.get_time_limit(
        args.search_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.search_memory_limit, args.overall_memory_limit)
    executable = get_executable(args.build, REL_SEARCH_PATH)

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
        try:
            call.check_call(
                "search",
                [executable] + args.search_options,
                stdin=args.search_input,
                time_limit=time_limit,
                memory_limit=memory_limit)
        except subprocess.CalledProcessError as err:
            # TODO: if we ever add support for SEARCH_PLAN_FOUND_AND_* directly
            # in the planner, this assertion no longer holds. Furthermore, we
            # would need to return (err.returncode, True) if the returncode is
            # in [0..10].
            # Negative exit codes are allowed for passing out signals.
            assert err.returncode >= 10 or err.returncode < 0, "got returncode < 10: {}".format(err.returncode)
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

    plan_files = list(PlanManager(args.plan_file).get_existing_plans())
    validate_inputs = [domain, task] + plan_files

    try:
        call.check_call(
            "validate",
            [VALIDATE] + validate_inputs,
            memory_limit=VALIDATE_MEMORY_LIMIT_IN_B)
    except OSError as err:
        if err.errno == errno.ENOENT:
            sys.exit("Error: {} not found. Is it on the PATH?".format(VALIDATE))
        else:
            raise
    else:
        return (0, True)
