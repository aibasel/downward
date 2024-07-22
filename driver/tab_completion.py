import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys

from . import returncodes
from . import util

try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False


def abort_tab_completion(warning):
    argcomplete.warn(warning)
    exit(returncodes.DRIVER_INPUT_ERROR)


def complete_build_arg(prefix, parsed_args, **kwargs):
    if parsed_args.debug:
        abort_tab_completion(
            "The option --debug is an alias for --build=debug. Do no specify "
            "both --debug and --build.")

    if not Path(util.BUILDS_DIR).exists():
        abort_tab_completion("No build exists.")
    return [p.name for p in Path(util.BUILDS_DIR).iterdir() if p.is_dir()]


def complete_planner_args(prefix, parsed_args, **kwargs):
    if parsed_args.build and parsed_args.debug:
        abort_tab_completion(
            "The option --debug is an alias for --build=debug. Do no specify "
            "both --debug and --build.")

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

    if parsed_args.filenames or double_dash_in_options:
        bin_dir = Path(util.BUILDS_DIR) / build / "bin"
        if current_mode == "search":
            if not last_option_was_mode_switch:
                completions.append("--translate-options")

            downward = bin_dir / "downward"
            if downward.exists():
                completions += get_completions_from_downward(
                    downward, parsed_args.search_options, prefix)
        else:
            if not last_option_was_mode_switch:
                completions.append("--search-options")

            translator = bin_dir / "translate" / "translate.py"
            if translator.exists():
                completions += get_completions_from_translator(
                    translator, parsed_args.translate_options, prefix)

    if has_only_filename_options and len(parsed_args.filenames) < 2:
        file_completer = argcomplete.FilesCompleter()
        completions += file_completer(prefix, **kwargs)

    return completions


def get_completions_from_downward(downward, options, prefix):
    simulated_commandline = [str(downward)] + options + [prefix]
    comp_line = " ".join(simulated_commandline)
    comp_point = str(len(comp_line))
    comp_cword = str(len(simulated_commandline) - 1)
    cmd = [str(downward), "--bash-complete",
           comp_point, comp_line, comp_cword] + simulated_commandline
    output = subprocess.check_output(cmd, text=True)
    return output.splitlines()


def get_completions_from_translator(translator, options, prefix):
    simulated_commandline = [str(translator)] + options + [prefix]
    comp_line = " ".join(simulated_commandline)
    comp_point = len(comp_line)

    translator_argument_path = translator.parent / "arguments.py"
    spec = importlib.util.spec_from_file_location("arguments", translator_argument_path)
    translator_arguments = importlib.util.module_from_spec(spec)
    sys.modules["arguments"] = translator_arguments
    spec.loader.exec_module(translator_arguments)

    # We create a new parser that will handle the autocompletion
    # for the translator
    translator_parser = argparse.ArgumentParser()
    translator_arguments.add_options(translator_parser)

    class CustomInputFinder(argcomplete.finders.CompletionFinder):
        def get_completions(self, comp_line, comp_point):
            cword_prequote, cword_prefix, _, comp_words, last_wordbreak_pos = argcomplete.lexers.split_line(comp_line, comp_point)
            return self._get_completions(comp_words, cword_prefix, cword_prequote, last_wordbreak_pos)

    translator_finder = CustomInputFinder(translator_parser)
    return translator_finder.get_completions(comp_line, comp_point)


def enable(parser):
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)
