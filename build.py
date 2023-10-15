#!/usr/bin/env python3

import errno
import os
import subprocess
import sys

import build_configs

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

def print_usage():
    script_name = os.path.basename(__file__)
    configs = []
    for name, args in sorted(CONFIGS.items()):
        if name == DEFAULT_CONFIG_NAME:
            name += " (default)"
        if name == DEBUG_CONFIG_NAME:
            name += " (default with --debug)"
        configs.append(name + "\n    " + " ".join(args))
    configs_string = "\n  ".join(configs)
    cmake_name = os.path.basename(CMAKE)
    generator_name = CMAKE_GENERATOR.lower()
    default_config_name = DEFAULT_CONFIG_NAME
    debug_config_name = DEBUG_CONFIG_NAME
    print(f"""Usage: {script_name} [BUILD [BUILD ...]] [--all] [--debug] [MAKE_OPTIONS]

Build one or more predefined build configurations of Fast Downward. Each build
uses {cmake_name} to compile the code using {generator_name} . Build configurations
differ in the parameters they pass to {cmake_name}. By default, the build uses all
available cores if this number can be determined. Use the "-j" option for
{cmake_name} to override this default behaviour.

Build configurations
  {configs_string}

--all         Alias to build all build configurations.
--debug       Alias to build the default debug build configuration.
--help        Print this message and exit.

Make options
  All other parameters are forwarded to the build step.

Example usage:
  ./{script_name}                     # build {default_config_name} in #cores threads
  ./{script_name} -j4                 # build {default_config_name} in 4 threads
  ./{script_name} debug               # build debug
  ./{script_name} --debug             # build {debug_config_name}
  ./{script_name} release debug       # build release and debug configs
  ./{script_name} --all VERBOSE=true  # build all build configs with detailed logs
""")


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


def main():
    config_names = []
    build_parameters = []
    for arg in sys.argv[1:]:
        if arg == "--help" or arg == "-h":
            print_usage()
            sys.exit(0)
        elif arg == "--debug":
            config_names.append(DEBUG_CONFIG_NAME)
        elif arg == "--all":
            config_names.extend(sorted(CONFIGS.keys()))
        elif arg in CONFIGS:
            config_names.append(arg)
        else:
            build_parameters.append(arg)
    if not config_names:
        config_names.append(DEFAULT_CONFIG_NAME)
    for config_name in config_names:
        build(config_name, CONFIGS[config_name], build_parameters)


if __name__ == "__main__":
    main()
