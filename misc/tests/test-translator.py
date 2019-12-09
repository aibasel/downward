#! /usr/bin/env python3


HELP = """\
Check that translator is deterministic.

Run the translator multiple times to test that the log and the output file are
the same for every run. Obviously, there might be false negatives, i.e.,
different runs might lead to the same nondeterministic results.
"""

import argparse
from collections import defaultdict
from distutils.spawn import find_executable
import itertools
import os
import re
import subprocess
import sys


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


def translate_task(task_file):
    python = sys.executable
    print("Translate {} with {}".format(get_task_name(task_file), python))
    sys.stdout.flush()
    cmd = [python, DRIVER, "--translate", task_file]
    try:
        output = subprocess.check_output(cmd)
    except OSError as err:
        sys.exit("Call failed: {}\n{}".format(" ".join(cmd), err))
    output = str(output)
    # Remove information that may differ between calls.
    for pattern in [
            r"\[.+s CPU, .+s wall-clock\]",
            r"\d+ KB"]:
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
        name not in blacklisted_domains]
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


def write_combined_output(output_file, task):
    log = translate_task(task)
    with open(output_file, "w") as combined_output:
        combined_output.write(log)
        with open("output.sas") as output_sas:
            combined_output.write(output_sas.read())


def main():
    args = parse_args()
    os.chdir(DIR)
    cleanup()
    subprocess.check_call(["./build.py", "translate"], cwd=REPO)
    for task in get_tasks(args):
        write_combined_output("base.sas", task)
        for iteration in range(2):
            write_combined_output("output{}.sas".format(iteration), task)
            print("Compare translator output", flush=True)
            files = ["base.sas", "output{}.sas".format(iteration)]
            try:
                subprocess.check_call(["diff", "-q"] + files)
            except subprocess.CalledProcessError:
                sys.exit(
                    "Error: Translator is nondeterministic for {task}.".format(**locals()))
            print(flush=True)
    cleanup()


if __name__ == "__main__":
    main()
