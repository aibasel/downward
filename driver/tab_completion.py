import argparse
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile

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

    completions = {}

    if can_use_double_dash:
        completions["--"] = ""

    if parsed_args.filenames or double_dash_in_options:
        bin_dir = Path(util.BUILDS_DIR) / build / "bin"
        if current_mode == "search":
            if not last_option_was_mode_switch:
                completions["--translate-options"] = ""

            downward = bin_dir / "downward"
            if downward.exists():
                completions.update(_get_completions_from_downward(
                    downward, parsed_args.search_options, prefix))
        else:
            if not last_option_was_mode_switch:
                completions["--search-options"] = ""

            translator = bin_dir / "translate" / "translate.py"
            if translator.exists():
                completions.update(_get_completions_from_translator(
                    translator, parsed_args.translate_options, prefix))

    if has_only_filename_options and len(parsed_args.filenames) < 2:
        file_completer = argcomplete.FilesCompleter()
        completions.update({f: "" for f in file_completer(prefix, **kwargs)})

    return completions


def _get_field_separators(env):
    entry_separator = env.get("IFS", "\n")
    help_separator = env.get("_ARGCOMPLETE_DFS")
    if env.get("_ARGCOMPLETE_SHELL") == "zsh":
        # Argcomplete always uses ":" on zsh, even if another value is set in
        # _ARGCOMPLETE_DFS.
        help_separator = ":"
    return entry_separator, help_separator


def _split_argcomplete_ouput(content, entry_separator, help_separator):
    suggestions = {}
    for line in content.split(entry_separator):
        if help_separator and help_separator in line:
            suggestion, help = line.split(help_separator, 1)
            suggestions[suggestion] = help
        else:
            suggestions[line] = ""
    return suggestions


def _call_argcomplete(python_file, comp_line, comp_point):
    with tempfile.NamedTemporaryFile(mode="r") as f:
        env = os.environ.copy()
        env["COMP_LINE"] = comp_line
        env["COMP_POINT"] = str(comp_point)
        env["_ARGCOMPLETE"] = "1"
        env["_ARGCOMPLETE_STDOUT_FILENAME"] = f.name
        subprocess.check_call([sys.executable, python_file], env=env)
        entry_separator, help_separator = _get_field_separators(env)
        return _split_argcomplete_ouput(f.read(), entry_separator, help_separator)

def _get_completions_from_downward(downward, options, prefix):
    entry_separator, help_separator = _get_field_separators(os.environ)
    help_separator = help_separator or ":"
    simulated_commandline = [str(downward)] + options + [prefix]
    comp_line = " ".join(simulated_commandline)
    comp_point = str(len(comp_line))
    comp_cword = str(len(simulated_commandline) - 1)
    cmd = [str(downward), "--bash-complete", entry_separator, help_separator,
           comp_point, comp_line, comp_cword] + simulated_commandline
    output = subprocess.check_output(cmd, text=True)
    return _split_argcomplete_ouput(output, entry_separator, help_separator)


def _get_completions_from_translator(translator, options, prefix):
    simulated_commandline = [str(translator), "dummy_domain", "dummy_problem"] + options + [prefix]
    comp_line = " ".join(simulated_commandline)
    comp_point = len(comp_line)
    return _call_argcomplete(translator, comp_line, comp_point)


def enable(parser):
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)
