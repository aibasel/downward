#! /usr/bin/env python3


HELP = """\
Check that translator is deterministic.
Run the translator multiple times to test that the log and the output file are
the same for every run. Obviously, there might be false negatives, i.e.,
different runs might lead to the same nondeterministic results.
"""

import argparse
from collections import defaultdict
import itertools
import os
from pathlib import Path
import re
import subprocess
import sys


DIR = Path(__file__).resolve().parent
REPO = DIR.parents[1]
DRIVER = REPO / "fast-downward.py"


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
    parser.add_argument(
        "--runs-per-task",
        help="translate each task this many times and compare the outputs",
        type=int, default=3)
    args = parser.parse_args()
    args.benchmarks_dir = Path(args.benchmarks_dir).resolve()
    return args


def get_task_name(path):
    return "-".join(str(path).split("/")[-2:])


def translate_task(task_file):
    print(f"Translate {get_task_name(task_file)}", flush=True)
    sys.stdout.flush()
    cmd = [sys.executable, str(DRIVER), "--translate", str(task_file)]
    try:
        output = subprocess.check_output(cmd, encoding=sys.getfilesystemencoding())
    except OSError as err:
        sys.exit(f"Call failed: {' '.join(cmd)}\n{err}")
    # Remove information that may differ between calls.
    for pattern in [
            r"\[(.+s CPU, .+s wall-clock)\]",
            r"(\d+) KB",
            r"Planner time: (.+s)",
            ]:
        output = re.sub(pattern, "XXX", output)
    return output


def _get_all_tasks_by_domain(benchmarks_dir):
    # Ignore domains where translating the first task takes too much time or memory.
    # We also ignore citycar, which indeed reveals some nondeterminism in the
    # invariant synthesis. Fixing it would require to sort the actions which
    # seems to be detrimental on some other domains.
    blacklisted_domains = [
        "agricola-sat18-strips",
        "citycar-opt14-adl",  # cf. issue879
        "citycar-sat14-adl",  # cf. issue879
        "organic-synthesis-sat18-strips",
        "organic-synthesis-split-opt18-strips",
        "organic-synthesis-split-sat18-strips"]
    benchmarks_dir = Path(benchmarks_dir)
    tasks = defaultdict(list)
    domains = [
        domain_dir for domain_dir in benchmarks_dir.iterdir()
        if domain_dir.is_dir() and
        not str(domain_dir.name).startswith((".", "_", "unofficial")) and
        str(domain_dir.name) not in blacklisted_domains]
    for domain in domains:
        path = benchmarks_dir / domain
        tasks[domain] = [
            benchmarks_dir / domain / f
            for f in sorted(path.iterdir()) if "domain" not in str(f)]
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
            suite.append(args.benchmarks_dir / task)
    return sorted(set(suite))


def cleanup():
    for f in Path(".").glob("translator-output-*.txt"):
        f.unlink()


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
    for task in get_tasks(args):
        base_file = "translator-output-0.txt"
        write_combined_output(base_file, task)
        for i in range(1, args.runs_per_task):
            compared_file = f"translator-output-{i}.txt"
            write_combined_output(compared_file, task)
            files = [base_file, compared_file]
            try:
                subprocess.check_call(["diff", "-q"] + files)
            except subprocess.CalledProcessError:
                sys.exit(f"Error: Translator is nondeterministic for {task}.")
        print("Outputs match\n", flush=True)
        cleanup()


if __name__ == "__main__":
    main()
