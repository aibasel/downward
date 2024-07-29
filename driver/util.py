import os
from pathlib import Path

from . import returncodes


DRIVER_DIR = Path(__file__).parent.resolve()
REPO_ROOT_DIR = DRIVER_DIR.parent
BUILDS_DIR = REPO_ROOT_DIR / "builds"


def get_elapsed_time():
    """
    Return the CPU time taken by the python process and its child
    processes.
    """
    if os.name == "nt":
        # The child time components of os.times() are 0 on Windows.
        raise NotImplementedError("cannot use get_elapsed_time() on Windows")
    return sum(os.times()[:4])


def find_domain_path(task_path: Path):
    """
    Find domain path for the given task using automatic naming rules.
    """
    domain_basenames = [
        "domain.pddl",
        task_path.stem + "-domain" + task_path.suffix,
        task_path.name[:3] + "-domain.pddl", # for airport
        "domain_" + task_path.name,
        "domain-" + task_path.name,
    ]

    for domain_basename in domain_basenames:
        domain_path = task_path.parent / domain_basename
        if domain_path.exists():
            return domain_path

    returncodes.exit_with_driver_input_error(
        "Error: Could not find domain file using automatic naming rules.")

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


def set_default_build(args):
    """If no build is specified, set args.build to the default build. This is
    typically 'release' but can be changed to 'debug' with the option
    '--debug'. This function modifies args directly."""
    if not args.build:
        if args.debug:
            args.build = "debug"
        else:
            args.build = "release"


def split_planner_args(args):
    """Partition args.planner_args, the list of arguments for the
    planner components, into args.filenames, args.translate_options
    and args.search_options. Modifies args directly.
    Returns the name of the last active component for tab completion."""

    args.filenames, options = _split_off_filenames(args.planner_args)

    args.translate_options = []
    args.search_options = []

    curr_options = args.search_options
    curr_option_name = "search"
    for option in options:
        if option == "--translate-options":
            curr_options = args.translate_options
            curr_option_name = "translate"
        elif option == "--search-options":
            curr_options = args.search_options
            curr_option_name = "search"
        else:
            curr_options.append(option)
    return curr_option_name
