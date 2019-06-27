#! /usr/bin/env python

from __future__ import print_function

HELP = """\
Check that translator is deterministic.

Run the translator on two different Python versions and test that the
log and the output file are the same. Obviously, there might be false
negatives, i.e., different Python versions might lead to the same
nondeterministic results.
"""

import argparse
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


def parse_args():
    parser = argparse.ArgumentParser(description=HELP)
    parser.add_argument(
        "benchmarks_dir",
        help="path to benchmark directory")
    parser.add_argument(
        "suite", nargs="*", default=["first"],
        help='Use "all" to test all benchmarks, '
             '"first" to test the first task of each domain (default), '
             'or "<domain>:<problem>" to test individual tasks')
    args = parser.parse_args()
    args.benchmarks_dir = os.path.abspath(args.benchmarks_dir)
    return args


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
            r".* command line string: .*/python\d(.\d)?"]:
        output = re.sub(pattern, "", output)
    return output


def _get_all_tasks_by_domain(benchmarks_dir):
    # Ignore domains where translating the first task takes too much time or memory.
    # We also ignore citycar, which indeed reveals some nondeterminism in the
    # invariant synthesis. Fixing it would require to sort the actions which
    # seems to be detrimental on some other domains.
    blacklisted_domains = [
        "agricola-sat18-strips",
        "citycar-opt14-adl", # cf. issue875
        "citycar-sat14-adl", # cf. issue875
        "organic-synthesis-sat18-strips",
        "organic-synthesis-split-opt18-strips",
        "organic-synthesis-split-sat18-strips"]
    tasks = defaultdict(list)
    domains = [
        name for name in os.listdir(benchmarks_dir)
        if os.path.isdir(os.path.join(benchmarks_dir, name)) and
        not name.startswith((".", "_")) and
        not name in blacklisted_domains]
    for domain in domains:
        path = os.path.join(benchmarks_dir, domain)
        tasks[domain] = [
            os.path.join(benchmarks_dir, domain, f)
            for f in sorted(os.listdir(path)) if "domain" not in f]
    return sorted(tasks.values())


def get_tasks(args):
    suite = []
    for task in args.suite:
        if task == "first":
            # Add the first task of each domain.
            suite.extend([tasks[0] for tasks in _get_all_tasks_by_domain(args.benchmarks_dir)])
        elif task == "all":
            # Add the whole benchmark suite.
            suite.extend(itertools.chain.from_iterable(
                    tasks for tasks in _get_all_tasks_by_domain(args.benchmarks_dir)))
        else:
            # Add task from command line.
            task = task.replace(":", "/")
            suite.append(os.path.join(args.benchmarks_dir, task))
    return sorted(set(suite))


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
    args = parse_args()
    os.chdir(DIR)
    cleanup()
    subprocess.check_call(["./build.py", "translate"], cwd=REPO)
    interpreter_paths = [get_abs_interpreter_path("python{}".format(version)) for version in VERSIONS]
    interpreter_versions = [get_python_version_info(path) for path in interpreter_paths]
    for task in get_tasks(args):
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
            subprocess.check_call(["diff", "-q"] + files)
        except subprocess.CalledProcessError:
            sys.exit(
                "Error: Translator is nondeterministic for {task}.".format(**locals()))
        print()
        sys.stdout.flush()
    cleanup()


if __name__ == "__main__":
    main()
