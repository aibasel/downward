import logging
import os
from pathlib import Path
import shutil
import subprocess
import sys

from . import call
from . import limits
from . import portfolio_runner
from . import returncodes
from . import util
from .plan_manager import PlanManager

if os.name == "posix":
    BINARY_EXT = ""
elif os.name == "nt":
    BINARY_EXT = ".exe"
else:
    returncodes.exit_with_driver_unsupported_error("Unsupported OS: " + os.name)

# TODO: We might want to turn translate into a module and call it with "python3 -m translate".
REL_TRANSLATE_PATH = Path("translate") / "translate.py"
REL_SEARCH_PATH = Path(f"downward{BINARY_EXT}")
# Older versions of VAL use lower case, newer versions upper case. We prefer the
# older version because this is what our build instructions recommend.
_VALIDATE_NAME = (shutil.which(f"validate{BINARY_EXT}") or
                  shutil.which(f"Validate{BINARY_EXT}"))
VALIDATE = Path(_VALIDATE_NAME) if _VALIDATE_NAME else None


def get_executable(build: str, rel_path: Path):
    # First, consider 'build' to be a path directly to the binaries.
    # The path can be absolute or relative to the current working
    # directory.
    build_dir = Path(build)
    if not build_dir.exists():
        # If build is not a full path to the binaries, it might be the
        # name of a build in our standard directory structure.
        # in this case, the binaries are in
        #   '<repo-root>/builds/<buildname>/bin'.
        build_dir = util.BUILDS_DIR / build / "bin"
        if not build_dir.exists():
            returncodes.exit_with_driver_input_error(
                f"Could not find build '{build}' at {build_dir}. "
                f"Please run './build.py {build}'.")

    abs_path = build_dir / rel_path
    if not abs_path.exists():
        returncodes.exit_with_driver_input_error(
            f"Could not find '{rel_path}' in build '{build}'. "
            f"Please run './build.py {build}'.")

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

    stderr, returncode = call.get_error_output_and_returncode(
        "translator",
        cmd,
        time_limit=time_limit,
        memory_limit=memory_limit)

    # We collect stderr of the translator and print it here, unless
    # the translator ran out of memory and all output in stderr is
    # related to MemoryError.
    do_print_on_stderr = True
    if returncode == returncodes.TRANSLATE_OUT_OF_MEMORY:
        output_related_to_memory_error = True
        if not stderr:
            output_related_to_memory_error = False
        for line in stderr.splitlines():
            if "MemoryError" not in line:
                output_related_to_memory_error = False
                break
        if output_related_to_memory_error:
            do_print_on_stderr = False

    if do_print_on_stderr and stderr:
        returncodes.print_stderr(stderr)

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


def run_search(args):
    logging.info("Running search (%s)." % args.build)
    time_limit = limits.get_time_limit(
        args.search_time_limit, args.overall_time_limit)
    memory_limit = limits.get_memory_limit(
        args.search_memory_limit, args.overall_memory_limit)
    executable = get_executable(args.build, REL_SEARCH_PATH)

    plan_manager = PlanManager(
        args.plan_file,
        portfolio_bound=args.portfolio_bound,
        single_plan=args.portfolio_single_plan)
    plan_manager.delete_existing_plans()

    if args.portfolio:
        assert not args.search_options
        logging.info(f"search portfolio: {args.portfolio}")
        return portfolio_runner.run(
            args.portfolio, executable, args.search_input, plan_manager,
            time_limit, memory_limit)
    else:
        if not args.search_options:
            returncodes.exit_with_driver_input_error(
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
    if not VALIDATE:
        returncodes.exit_with_driver_input_error(
            "Error: Trying to run validate but it was not found on the PATH.")

    logging.info("Running validate.")
    plan_files = list(PlanManager(args.plan_file).get_existing_plans())
    if not plan_files:
        print("Not running validate since no plans found.")
        return (0, True)

    try:
        call.check_call(
            "validate",
            [VALIDATE] + args.validate_inputs + plan_files,
            time_limit=args.validate_time_limit,
            memory_limit=args.validate_memory_limit)
    except OSError as err:
        returncodes.exit_with_driver_critical_error(err)
    else:
        return (0, True)
