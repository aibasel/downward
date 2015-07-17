# -*- coding: utf-8 -*-

import argparse
import os.path

from . import aliases
from . import limits
from .util import SRC_DIR


DESCRIPTION = """Fast Downward driver script.

Input files can be either a PDDL problem file (with an optional PDDL domain
file), in which case the driver runs all three planner components,  or a SAS+
preprocessor output file, in which case the driver runs just the search
component. This default behaviour can be overridden with the options below.

Arguments given before the specified input files are interpreted by the driver
script ("driver options"). Arguments given after the input files are passed on
to the planner components ("component options"). In exceptional cases where no
input files are needed, use "--" to separate driver from component options. In
even more exceptional cases where input files begin with "--", use "--" to
separate driver options from input files and also to separate input files from
component options.

By default, component options are passed to the search component. Use
"--translate-options", "--preprocess-options" or "--search-options" within the
component options to override the default for the following options, until
overridden again. (See below for examples.)"""

LIMITS_HELP = """You can limit the time or memory for individual components
or the whole planner. The effective limit for each component is the minimum
of the component, overall, external soft, and external hard limits.

Limits are given in seconds or MiB. You can change the unit by using the
suffixes s, m, h and K, M, G.

By default, all limits are inactive. Only external limits (e.g. set with
ulimit) are respected.

Portfolios require that a time limit is in effect. Portfolio configurations
that exceed their time or memory limit are aborted, and the next
configuration is run."""

EXAMPLE_PORTFOLIO = os.path.relpath(
    aliases.PORTFOLIOS["seq-opt-fdss-1"], start=SRC_DIR)

EXAMPLES = [
    ("Translate and preprocess, then find a plan with A* + LM-Cut:",
     ["./fast-downward.py", "../benchmarks/gripper/prob01.pddl",
      "--search", '"astar(lmcut())"']),
    ("Translate and preprocess, run no search:",
     ["./fast-downward.py", "--translate", "--preprocess",
      "../benchmarks/gripper/prob01.pddl"]),
    ("Run predefined configuration (LAMA-2011) on preprocessed task:",
     ["./fast-downward.py", "--alias", "seq-sat-lama-2011", "output"]),
    ("Run a portfolio on a preprocessed task:",
     ["./fast-downward.py", "--portfolio", EXAMPLE_PORTFOLIO,
      "--search-time-limit", "30m", "output"]),
    ("Run the search component in debug mode (with assertions enabled):",
     ["./fast-downward.py", "--debug", "output", "--search", '"astar(ipdb())"']),
    ("Pass options to translator and search components:",
     ["./fast-downward.py", "../benchmarks/gripper/prob01.pddl",
      "--translate-options", "--relaxed",
      "--search-options", "--search", '"astar(lmcut())"']),
]

EPILOG = """component options:
  --translate-options OPTION1 OPTION2 ...
  --preprocess-options OPTION1 OPTION2 ...
  --search-options OPTION1 OPTION2 ...
                        pass OPTION1 OPTION2 ... to specified planner component
                        (default: pass component options to search)

Examples:

%s
""" % "\n\n".join("%s\n%s" % (desc, " ".join(cmd)) for desc, cmd in EXAMPLES)

COMPONENTS_PLUS_OVERALL = ["translate", "preprocess", "search", "overall"]


class RawHelpFormatter(argparse.HelpFormatter):
    """Preserve newlines and spacing."""
    def _fill_text(self, text, width, indent):
        return ''.join([indent + line for line in text.splitlines(True)])

    def _format_args(self, action, default_metavar):
        """Show explicit help for remaining args instead of "..."."""
        if action.nargs == argparse.REMAINDER:
            return "INPUT_FILE1 [INPUT_FILE2] [COMPONENT_OPTION ...]"
        else:
            return argparse.HelpFormatter._format_args(self, action, default_metavar)


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

    if "--" in planner_args:
        separator_pos = _rindex(planner_args, "--")
        num_filenames = separator_pos
        del planner_args[separator_pos]
    else:
        num_filenames = 0
        for arg in planner_args:
            # We treat "-" by itself as a filename because by common
            # convention it denotes stdin or stdout, and we might want
            # to support this later.
            if arg.startswith("-") and arg != "-":
                break
            num_filenames += 1
    return planner_args[:num_filenames], planner_args[num_filenames:]


def _split_planner_args(parser, args):
    """Partition args.planner_args, the list of arguments for the
    planner components, into args.filenames, args.translate_options,
    arge.preprocess_options and args.search_options. Modifies args
    directly and removes the original args.planner_args list."""

    args.filenames, options = _split_off_filenames(args.planner_args)

    args.translate_options = []
    args.preprocess_options = []
    args.search_options = []

    curr_options = args.search_options
    for option in options:
        if option == "--translate-options":
            curr_options = args.translate_options
        elif option == "--preprocess-options":
            curr_options = args.preprocess_options
        elif option == "--search-options":
            curr_options = args.search_options
        else:
            curr_options.append(option)


def _check_mutex_args(parser, args, required=False):
    for pos, (name1, is_specified1) in enumerate(args):
        for name2, is_specified2 in args[pos + 1:]:
            if is_specified1 and is_specified2:
                parser.error("cannot combine %s with %s" % (name1, name2))
    if required and not any(is_specified for _, is_specified in args):
        parser.error("exactly one of {%s} has to be specified" %
            ", ".join(name for name, _ in args))


def _looks_like_search_input(filename):
    # We don't currently have a good way to distinguish preprocess and
    # search inputs without going through most of the file, so we
    # don't even try.
    with open(filename) as input_file:
        first_line = next(input_file, "").rstrip()
    return first_line == "begin_version"


def _set_components_automatically(parser, args):
    """Guess which planner components to run based on the specified
    filenames and set args.components accordingly. Currently
    implements some simple heuristics:

    1. If there is exactly one input file and it looks like a
       Fast-Downward-generated file, run search only.
    2. Otherwise, run all components."""

    if len(args.filenames) == 1 and _looks_like_search_input(args.filenames[0]):
        args.components = ["search"]
    else:
        args.components = ["translate", "preprocess", "search"]


def _set_components_and_inputs(parser, args):
    """Set args.components to the planner components to be run
    and set args.translate_inputs, args.preprocess_input and
    args.search_input to the correct input filenames.

    Rules:
    1. If any --run-xxx option is specified, then the union
       of the specified components is run.
    2. It is an error to specify running the translator and
       search, but not the preprocessor.
    3. If nothing is specified, use automatic rules. See
       separate function."""

    args.components = []
    if args.translate or args.run_all:
        args.components.append("translate")
    if args.preprocess or args.run_all:
        args.components.append("preprocess")
    if args.search or args.run_all:
        args.components.append("search")

    if args.components == ["translate", "search"]:
        parser.error("cannot run translator and search without preprocessor")

    if not args.components:
        _set_components_automatically(parser, args)

    args.translate_inputs = []
    args.preprocess_input = "output.sas"
    args.search_input = "output"

    assert args.components
    first = args.components[0]
    num_files = len(args.filenames)
    # When passing --help to any of the components (or -h to the
    # translator), we don't require input filenames and silently
    # swallow any that are provided. This is undocumented to avoid
    # cluttering the driver's --help output.
    if first == "translate":
        if "--help" in args.translate_options or "-h" in args.translate_options:
            args.translate_inputs = []
        elif num_files in [1, 2]:
            args.translate_inputs = args.filenames
        else:
            parser.error("translator needs one or two input files")
    elif first == "preprocess":
        if "--help" in args.preprocess_options:
            args.preprocess_input = None
        elif num_files == 1:
            args.preprocess_input, = args.filenames
        else:
            parser.error("preprocessor needs exactly one input file")
    elif first == "search":
        if "--help" in args.search_options:
            args.search_input = None
        elif num_files == 1:
            args.search_input, = args.filenames
        else:
            parser.error("search needs exactly one input file")
    else:
        assert False, first


def _convert_limits_to_ints(parser, args):
    for component in COMPONENTS_PLUS_OVERALL:
        limits.set_time_limit_in_seconds(parser, args, component)
        limits.set_memory_limit_in_bytes(parser, args, component)


def parse_args():
    parser = argparse.ArgumentParser(
        description=DESCRIPTION, epilog=EPILOG,
        formatter_class=RawHelpFormatter,
        add_help=False)

    help_options = parser.add_argument_group(
        title=("driver options that show information and exit "
               "(don't run planner)"))
    # We manually add the help option because we want to control
    # how it is grouped in the output.
    help_options.add_argument(
        "-h", "--help",
        action="help", default=argparse.SUPPRESS,
        help="show this help message and exit")
    help_options.add_argument(
        "--show-aliases", action="store_true",
        help="show the known aliases (see --alias) and exit")

    components = parser.add_argument_group(
        title=("driver options selecting the planner components to be run\n"
               "(may select several; default: auto-select based on input file(s))"))
    components.add_argument(
        "--run-all", action="store_true",
        help="run all components of the planner")
    components.add_argument(
        "--translate", action="store_true",
        help="run translator component")
    components.add_argument(
        "--preprocess", action="store_true",
        help="run preprocessor component")
    components.add_argument(
        "--search", action="store_true",
        help="run search component")

    limits = parser.add_argument_group(
        title="time and memory limits", description=LIMITS_HELP)
    for component in COMPONENTS_PLUS_OVERALL:
        limits.add_argument("--{}-time-limit".format(component))
        limits.add_argument("--{}-memory-limit".format(component))

    driver_other = parser.add_argument_group(
        title="other driver options")
    driver_other.add_argument(
        "--alias",
        help="run a config with an alias (e.g. seq-sat-lama-2011)")
    driver_other.add_argument(
        "--debug", action="store_true",
        help="use debug mode for search component")
    driver_other.add_argument(
        "--log-level", choices=["debug", "info", "warning"],
        default="info",
        help="set log level (most verbose: debug; least verbose: warning; default: %(default)s)")

    driver_other.add_argument(
        "--plan-file", metavar="FILE", default="sas_plan",
        help="write plan(s) to FILE (default: %(default)s; anytime configurations append .1, .2, ...)")
    driver_other.add_argument(
        "--portfolio", metavar="FILE",
        help="run a portfolio specified in FILE")

    parser.add_argument(
        "planner_args", nargs=argparse.REMAINDER,
        help="file names and options passed on to planner components")

    # Using argparse.REMAINDER relies on the fact that the first
    # argument that doesn't belong to the driver doesn't look like an
    # option, i.e., doesn't start with "-". This is usually satisfied
    # because the argument is a filename; in exceptional cases, "--"
    # can be used as an explicit separator. For example, "./fast-downward.py --
    # --help" passes "--help" to the search code.

    args = parser.parse_args()

    _split_planner_args(parser, args)

    _check_mutex_args(parser, [
            ("--alias", args.alias is not None),
            ("--portfolio", args.portfolio is not None),
            ("options for search component", bool(args.search_options))])

    _convert_limits_to_ints(parser, args)

    if args.alias:
        try:
            aliases.set_options_for_alias(args.alias, args)
        except KeyError:
            parser.error("unknown alias: %r" % args.alias)

    if not args.show_aliases:
        _set_components_and_inputs(parser, args)

    return args
