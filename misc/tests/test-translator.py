#! /usr/bin/env python

"""
Run the translator on supported Python versions and test that the log
and the output file are the same for all versions.

# Run small test on a few problems.
./test-translator.py

# Run bigger test on the first problem of each domain.
./test-translator.py first

# Run even bigger test on the whole benchmark suite.
./test-translator.py all

# Run test on specific problems.
./test-translator.py gripper:prob01.pddl depot:pfile1
"""

from __future__ import print_function

from collections import defaultdict
from distutils.spawn import find_executable
import itertools
import os
import re
import subprocess
import sys


VERSIONS = ["2.7", "3"]

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
DRIVER = os.path.join(REPO, "fast-downward.py")
BENCHMARKS = os.path.join(REPO, "benchmarks")

# Translating these problems covers 84% of the translator code. Translating all
# first problems covers 85%.
TASKS = [
    "airport:p01-airport1-p1.pddl",
    "airport-adl:p01-airport1-p1.pddl",
    "assembly:prob01.pddl",
    "barman-opt11-strips:pfile01-001.pddl",
    "miconic-fulladl:f1-0.pddl",
    "psr-large:p01-s29-n2-l5-f30.pddl",
    "transport-opt08-strips:p01.pddl",
    "woodworking-opt11-strips:p01.pddl",
    ]


def get_task_name(path):
    return "-".join(path.split("/")[-2:])


def translate_task(python, python_version, task_file):
    print("Translate {} with {} ({})".format(
        get_task_name(task_file), python, python_version))
    sys.stdout.flush()
    cmd = [python, DRIVER, "--translate", task_file]
    env = os.environ.copy()
    env["PYTHONHASHSEED"] = "random"
    try:
        output = subprocess.check_output(cmd, env=env)
    except OSError as err:
        sys.exit("Call failed: {}\n{}".format(" ".join(cmd), err))
    output = str(output)
    # Remove information that may differ between calls.
    for pattern in [
            r"\[.+s CPU, .+s wall-clock\]",
            r"\d+ KB",
            r"callstring: .*/python\d(.\d)?"]:
        output = re.sub(pattern, "", output)
    return output


def _get_all_tasks_by_domain():
    tasks = defaultdict(list)
    for domain in os.listdir(BENCHMARKS):
        path = os.path.join(BENCHMARKS, domain)
        tasks[domain] = [os.path.join(BENCHMARKS, domain, f)
                         for f in sorted(os.listdir(path)) if not "domain" in f]
    return tasks


def get_tasks():
    if "first" in sys.argv:
        # Use the first problem of each domain.
        return [tasks[0] for tasks in sorted(_get_all_tasks_by_domain().values())]

    if "all" in sys.argv:
        # Use the whole benchmark suite.
        return itertools.chain.from_iterable(
                tasks for tasks in _get_all_tasks_by_domain().values())

    # Use the problems given on the commandline.
    tasks = [arg for arg in sys.argv[1:] if not arg.startswith("--")]
    if not tasks:
        # Use predefined problems.
        tasks = TASKS

    # Make the paths absolute.
    return [os.path.join(BENCHMARKS, *task.split(":")) for task in tasks]


def cleanup():
    # We can't use the driver's cleanup function since the files are renamed.
    for f in os.listdir("."):
        if f.endswith(".sas"):
            os.remove(f)


def get_abs_interpreter_path(python_name):
    # For some reason, the driver cannot find the Python executable
    # (sys.executable returns the empty string) if we don't use the
    # absolute path here.
    abs_python = find_executable(python_name)
    if not abs_python:
        sys.exit("Error: {} couldn't be found.".format(python_name))
    return abs_python


def get_python_version_info(python):
    output = subprocess.check_output(
        [python, '-V'],
        stderr=subprocess.STDOUT,
        universal_newlines=True).strip()
    assert output.startswith("Python "), output
    return output[len("Python "):]


def main():
    os.chdir(DIR)
    cleanup()
    interpreter_paths = [get_abs_interpreter_path("python{}".format(version)) for version in VERSIONS]
    interpreter_versions = [get_python_version_info(path) for path in interpreter_paths]
    for task in get_tasks():
        for python, python_version in zip(interpreter_paths, interpreter_versions):
            log = translate_task(python, python_version, task)
            with open(python_version + ".sas", "w") as combined_output:
                combined_output.write(log)
                with open("output.sas") as output_sas:
                    combined_output.write(output_sas.read())
        print("Compare translator output")
        sys.stdout.flush()
        assert len(interpreter_versions) == 2, "Code only tests difference between 2 versions"
        files = [version + ".sas" for version in interpreter_versions]
        try:
            subprocess.check_call(["diff"] + files)
        except subprocess.CalledProcessError:
            sys.exit(
                "Error: Translator output for %s differs between Python versions. "
                "See above diff or compare the files %s in %s." % (task, files, DIR))
        print()
        sys.stdout.flush()
    cleanup()


if __name__ == "__main__":
    main()
