#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

import argparse
import errno
import os
import subprocess
import sys

import build_configs

try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False

CONFIGS = {config: params for config, params in build_configs.__dict__.items()
           if not config.startswith("_")}
DEFAULT_CONFIG_NAME = CONFIGS.pop("DEFAULT")
DEBUG_CONFIG_NAME = CONFIGS.pop("DEBUG")
CMAKE = "cmake"
CMAKE_GENERATOR = None
if os.name == "posix":
    CMAKE_GENERATOR = "Unix Makefiles"
elif os.name == "nt":
    CMAKE_GENERATOR = "NMake Makefiles"
try:
    # Number of usable CPUs (Unix only)
    NUM_CPUS = len(os.sched_getaffinity(0))
except AttributeError:
    # Number of available CPUs as a fall-back (may be None)
    NUM_CPUS = os.cpu_count()


class RawHelpFormatter(argparse.HelpFormatter):
    """Preserve newlines and spacing."""
    def _fill_text(self, text, width, indent):
        return ''.join([indent + line for line in text.splitlines(True)])

    def _format_args(self, action, default_metavar):
        """Show explicit help for remaining args instead of "..."."""
        if action.nargs == argparse.REMAINDER:
            return "[BUILD ...] [CMAKE_OPTION ...]"
        else:
            return argparse.HelpFormatter._format_args(self, action, default_metavar)


def parse_args():
    description = """Build one or more predefined build configurations of Fast Downward. Each build
uses CMake to compile the code. Build configurations differ in the parameters
they pass to CMake. By default, the build uses all available cores if this
number can be determined. Use the "-j" option for CMake to override this
default behaviour.
"""
    script_name = os.path.basename(__file__)
    configs = []
    for name, args in sorted(CONFIGS.items()):
        if name == DEFAULT_CONFIG_NAME:
            name += " (default)"
        if name == DEBUG_CONFIG_NAME:
            name += " (default with --debug)"
        configs.append(name + "\n    " + " ".join(args))
    configs_string = "\n  ".join(configs)
    epilog = f"""build configurations:
  {configs_string}

example usage:
  {script_name}                     # build {DEFAULT_CONFIG_NAME} with #cores parallel processes
  {script_name} -j4                 # build {DEFAULT_CONFIG_NAME} with 4 parallel processes
  {script_name} debug               # build debug
  {script_name} --debug             # build {DEBUG_CONFIG_NAME}
  {script_name} release debug       # build release and debug configs
  {script_name} --all VERBOSE=true  # build all build configs with detailed logs
"""

    parser = argparse.ArgumentParser(
        description=description, epilog=epilog, formatter_class=RawHelpFormatter)
    parser.add_argument("--debug", action="store_true",
                        help="alias to build the default debug build configuration")
    parser.add_argument("--all", action="store_true",
                        help="alias to build all build configurations")
    remaining_args = parser.add_argument("arguments", nargs=argparse.REMAINDER,
                        help="build configurations (see below) or build options")
    remaining_args.completer = complete_arguments

    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(parser)
    args = parser.parse_args()
    return args


def complete_arguments(prefix, parsed_args, **kwargs):
    split_args(parsed_args)
    unused_configs = set(CONFIGS) - set(parsed_args.config_names)
    return sorted([c for c in unused_configs if c.startswith(prefix)])


def get_project_root_path():
    import __main__
    return os.path.dirname(__main__.__file__)


def get_builds_path():
    return os.path.join(get_project_root_path(), "builds")


def get_src_path():
    return os.path.join(get_project_root_path(), "src")


def get_build_path(config_name):
    return os.path.join(get_builds_path(), config_name)


def try_run(cmd):
    print(f'Executing command "{" ".join(cmd)}"')
    try:
        subprocess.check_call(cmd)
    except OSError as exc:
        if exc.errno == errno.ENOENT:
            print(f"Could not find '{cmd[0]}' on your PATH. For installation instructions, "
                  "see BUILD.md in the project root directory.")
            sys.exit(1)
        else:
            raise


def build(config_name, configure_parameters, build_parameters):
    print(f"Building configuration {config_name}.")

    build_path = get_build_path(config_name)
    generator_cmd = [CMAKE, "-S", get_src_path(), "-B", build_path]
    if CMAKE_GENERATOR:
        generator_cmd += ["-G", CMAKE_GENERATOR]
    generator_cmd += configure_parameters
    try_run(generator_cmd)

    build_cmd = [CMAKE, "--build", build_path]
    if NUM_CPUS:
        build_cmd += ["-j", f"{NUM_CPUS}"]
    if build_parameters:
        build_cmd += ["--"] + build_parameters
    try_run(build_cmd)

    print(f"Built configuration {config_name} successfully.")


def split_args(args):
    args.config_names = []
    args.build_parameters = []
    for arg in args.arguments:
        if arg in CONFIGS:
            args.config_names.append(arg)
        else:
            args.build_parameters.append(arg)

    if args.debug:
        args.config_names.append(DEBUG_CONFIG_NAME)
    if args.all:
        args.config_names.extend(sorted(CONFIGS.keys()))


def main():
    args = parse_args()
    split_args(args)
    for config_name in args.config_names or [DEFAULT_CONFIG_NAME]:
        build(config_name, CONFIGS[config_name],
              args.build_parameters)


if __name__ == "__main__":
    main()
