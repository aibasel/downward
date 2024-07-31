import os
import subprocess
import sys
import tempfile

from . import util
from .run_components import get_search_command, get_translate_command, MissingBuildError

try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False


def complete_build_arg(prefix, parsed_args, **kwargs):
    try:
        return [p.name for p in util.BUILDS_DIR.iterdir() if p.is_dir()]
    except OSError:
        return []


def complete_planner_args(prefix, parsed_args, **kwargs):
    util.set_default_build(parsed_args)

    # Get some information from planner_args before it is deleted in split_planner_args().
    planner_args = parsed_args.planner_args
    num_planner_args = len(planner_args)
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
        if current_mode == "search":
            completions["--translate-options"] = ""
            completions.update(_get_completions_from_downward(
                parsed_args.build, parsed_args.search_options, prefix))
        else:
            completions["--search-options"] = ""
            completions.update(_get_completions_from_translator(
                parsed_args.build, parsed_args.translate_options, prefix))

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


def _split_argcomplete_output(content, entry_separator, help_separator):
    suggestions = {}
    for line in content.split(entry_separator):
        if help_separator and help_separator in line:
            suggestion, help = line.split(help_separator, 1)
            suggestions[suggestion] = help
        else:
            suggestions[line] = ""
    return suggestions


def _get_bash_completion_args(cmd, options, prefix):
    """
    Return values for four environment variables, bash uses as part of tab
    completion when cmd is called with parsed arguments args, and the unparsed
    prefix of a word to be completed prefix.
    COMP_POINT: the cursor position within COMP_LINE
    COMP_LINE: the full command line as a string
    COMP_CWORD: an index into COMP_WORDS to the word under the cursor.
    COMP_WORDS: the command line as list of words
    """
    comp_words = [str(x) for x in cmd] + options + [prefix]
    comp_line = " ".join(comp_words)
    comp_point = str(len(comp_line))
    comp_cword = str(len(comp_words) - 1)
    return comp_point, comp_line, comp_cword, comp_words


def _call_argcomplete(cmd, comp_line, comp_point):
    with tempfile.NamedTemporaryFile(mode="r") as f:
        env = os.environ.copy()
        env["COMP_LINE"] = comp_line
        env["COMP_POINT"] = comp_point
        env["_ARGCOMPLETE"] = "1"
        env["_ARGCOMPLETE_STDOUT_FILENAME"] = f.name
        subprocess.check_call(cmd, env=env)
        entry_separator, help_separator = _get_field_separators(env)
        return _split_argcomplete_output(f.read(), entry_separator, help_separator)

def _get_completions_from_downward(build, options, prefix):
    try:
        search_command = get_search_command(build)
    except MissingBuildError:
        return {}

    entry_separator, help_separator = _get_field_separators(os.environ)
    help_separator = help_separator or ":"
    comp_point, comp_line, comp_cword, comp_words = _get_bash_completion_args(
        search_command, options, prefix)
    cmd = [str(x) for x in search_command] + ["--bash-complete", entry_separator, help_separator,
           comp_point, comp_line, comp_cword] + comp_words
    output = subprocess.check_output(cmd, text=True)
    return _split_argcomplete_output(output, entry_separator, help_separator)


def _get_completions_from_translator(build, options, prefix):
    try:
        translate_command = get_translate_command(build)
    except MissingBuildError:
        return {}

    # We add domain and problem as dummy file names because otherwise, the
    # translators tab completion will suggest filenames which we don't want at
    # this point. Technically, file names should come after any options we
    # complete but to consider them in the completion, they have to be to the
    # left of the cursor position and to the left of any options that take
    # parameters.
    comp_point, comp_line, _, _ = _get_bash_completion_args(
        translate_command, ["domain", "problem"] + options, prefix)
    return _call_argcomplete(translate_command, comp_line, comp_point)


def enable(parser):
    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)
