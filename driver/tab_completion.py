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


def _rindex(seq, element):
    """Like list.index, but gives the index of the *last* occurrence."""
    seq = list(reversed(seq))
    reversed_index = seq.index(element)
    return len(seq) - 1 - reversed_index


def _split_off_filenames(planner_args):
    """Given the list of arguments to be passed on to the planner
    components, split it into a prefix of filenames and a suffix of
    options. Returns a pair (filenames, options).

    If a "--" separator is present, the last such separator serves as
    the border between filenames and options. The separator itself is
    not returned. (This implies that "--" can be a filename, but never
    an option to a planner component.)

    If no such separator is present, the first argument that begins
    with "-" and consists of at least two characters starts the list
    of options, and all previous arguments are filenames."""

    double_dash_in_options = False
    if "--" in planner_args:
        separator_pos = _rindex(planner_args, "--")
        num_filenames = separator_pos
        del planner_args[separator_pos]
        double_dash_in_options = True
    else:
        num_filenames = 0
        for arg in planner_args:
            # We treat "-" by itself as a filename because by common
            # convention it denotes stdin or stdout, and we might want
            # to support this later.
            if arg.startswith("-") and arg != "-":
                break
            num_filenames += 1
    return planner_args[:num_filenames], planner_args[num_filenames:], double_dash_in_options


def _build_arg_completion(prefix, parsed_args, **kwargs):
    if parsed_args.debug:
        argcomplete.warn("The option --debug is an alias for --build=debug. Do no specify both --debug and --build.")
        exit(1)

    builds_folder = Path(util.REPO_ROOT_DIR) / "builds"
    if not builds_folder.exists():
        argcomplete.warn("No build exists.")
        exit(1)
    return [p.name for p in builds_folder.iterdir() if p.is_dir()]


def add_build_arg_completer(build_arg):
    build_arg.completer = _build_arg_completion


def _planner_args_completion(prefix, parsed_args, **kwargs):
    if HAS_ARGCOMPLETE:
        if parsed_args.build and parsed_args.debug:
            argcomplete.warn("The option --debug is an alias for --build=debug. Do no specify both --debug and --build.")
            exit(1)

        build = parsed_args.build
        if not build:
            if parsed_args.debug:
                build = "debug"
            else:
                build = "release"

        filenames, options, double_dash_in_options = _split_off_filenames(parsed_args.planner_args)

        completions = []

        if not options and not (len(filenames) == 2 and double_dash_in_options):
            completions.append("--")

        translate_options = []
        search_options = []
        last_option_was_mode_switch = False

        curr_options = search_options
        for option in options:
            if option == "--translate-options":
                curr_options = translate_options
                last_option_was_mode_switch = True
            elif option == "--search-options":
                curr_options = search_options
                last_option_was_mode_switch = True
            else:
                curr_options.append(option)
                last_option_was_mode_switch = False

        if filenames:
            if curr_options is search_options:
                if not last_option_was_mode_switch:
                    completions.append("--translate-options")

                downward = Path(util.REPO_ROOT_DIR) / "builds" / build / "bin" / "downward"
                if downward.exists():
                    simulated_commandline = [str(downward)] + search_options + [prefix]
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

        if len(filenames) < 2 and not double_dash_in_options and not translate_options and not search_options and not last_option_was_mode_switch:
            file_completer = argcomplete.FilesCompleter()
            completions += file_completer(prefix, **kwargs)

        return completions


def add_planner_args_completer(planner_args):
    planner_args.completer = _planner_args_completion


def enable(parser):
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)
