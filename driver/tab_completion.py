import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys

from . import util

try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False


def complete_build_arg(prefix, parsed_args, **kwargs):
    if parsed_args.debug:
        argcomplete.warn("The option --debug is an alias for --build=debug. Do no specify both --debug and --build.")
        exit(1)

    builds_folder = Path(util.REPO_ROOT_DIR) / "builds"
    if not builds_folder.exists():
        argcomplete.warn("No build exists.")
        exit(1)
    return [p.name for p in builds_folder.iterdir() if p.is_dir()]


def complete_planner_args(prefix, parsed_args, **kwargs):
    if parsed_args.build and parsed_args.debug:
        argcomplete.warn("The option --debug is an alias for --build=debug. Do no specify both --debug and --build.")
        exit(1)

    build = parsed_args.build
    if not build:
        if parsed_args.debug:
            build = "debug"
        else:
            build = "release"

    # Get some information from planner_args before it is deleted in split_planner_args().
    planner_args = parsed_args.planner_args
    num_planner_args = len(planner_args)
    mode_switches = ["--translate-options", "--search-options"]
    last_option_was_mode_switch = planner_args and (planner_args[-1] in mode_switches)
    double_dash_in_options = "--" in planner_args

    current_mode = util.split_planner_args(parsed_args)
    num_filenames = len(parsed_args.filenames)
    has_only_filename_options = (num_filenames == num_planner_args)
    has_only_filename_or_double_dash_options = (num_filenames + int(double_dash_in_options) == num_planner_args)
    can_use_double_dash = (1 <= num_planner_args <= 2) and has_only_filename_or_double_dash_options

    completions = []

    if can_use_double_dash:
        completions.append("--")

    if parsed_args.filenames:
        if current_mode == "search":
            if not last_option_was_mode_switch:
                completions.append("--translate-options")

            downward = Path(util.REPO_ROOT_DIR) / "builds" / build / "bin" / "downward"
            if downward.exists():
                simulated_commandline = [str(downward)] + parsed_args.search_options + [prefix]
                comp_line = " ".join(simulated_commandline)
                comp_point = str(len(comp_line))
                comp_cword = str(len(simulated_commandline) - 1)
                cmd = [str(downward), "--bash-complete",
                        comp_point, comp_line, comp_cword] + simulated_commandline
                output = subprocess.check_output(cmd, text=True)
                completions += output.split()
        else:
            tranlator_arguments_path = Path(util.REPO_ROOT_DIR) / "builds" / build / "bin" / "translate" / "arguments.py"
            if tranlator_arguments_path.exists():
                spec = importlib.util.spec_from_file_location("arguments", tranlator_arguments_path)
                tranlator_arguments = importlib.util.module_from_spec(spec)
                sys.modules["arguments"] = tranlator_arguments
                spec.loader.exec_module(tranlator_arguments)

                # We create a new parser that will handle the autocompletion
                # for the translator
                translator_parser = argparse.ArgumentParser()
                tranlator_arguments.add_options(translator_parser)
                if not last_option_was_mode_switch:
                    translator_parser.add_argument("--search-options", action="store_true")
                argcomplete.autocomplete(translator_parser)

    if has_only_filename_options and len(parsed_args.filenames) < 2:
        file_completer = argcomplete.FilesCompleter()
        completions += file_completer(prefix, **kwargs)

    return completions


def enable(parser):
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)
