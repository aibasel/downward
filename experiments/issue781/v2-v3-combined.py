#! /usr/bin/env python

from collections import defaultdict
import os.path
import sys

import common_setup

FILE = os.path.abspath(__file__)
DIR = os.path.dirname(FILE)

FILENAME = os.path.splitext(os.path.basename(__file__))[0]
EXPS = os.path.join(DIR, "data")
EXPPATH = os.path.join(EXPS, FILENAME)

def remove_file(filename):
    try:
        os.remove(filename)
    except OSError:
        pass

exp = common_setup.IssueExperiment()
exp.steps = []
exp.add_step(
    'remove-combined-properties',
    remove_file,
    os.path.join(exp.eval_dir, "properties"))

exp.add_fetcher(os.path.join(EXPS, "issue781-v2-eval"), merge=True)
exp.add_fetcher(os.path.join(EXPS, "issue781-v3-queue-ratio-eval"), merge=True)

ATTRIBUTES = [
    "cost", "error", "run_dir", "search_start_time",
    "search_start_memory", "coverage", "expansions_until_last_jump",
    "total_time", "initial_h_value", "search_time", "abstractions",
    "stored_heuristics", "stored_values", "stored_lookup_tables",
]
exp.add_absolute_report_step(
    filter_algorithm=[
        "issue781-v2-blind-ec-min-0.0",
        "issue781-v2-blind-ec-min-0.2",
        "issue781-v2-blind-queue-min-0.0",
        "issue781-v3-blind-queue-min-0.2",
        "issue781-v2-blind-simple-min-0.0",
        "issue781-v2-blind-simple-min-0.2",
        "issue781-v2-lmcut-ec-min-0.0",
        "issue781-v2-lmcut-ec-min-0.2",
        "issue781-v2-lmcut-queue-min-0.0",
        "issue781-v3-lmcut-queue-min-0.2",
        "issue781-v2-lmcut-simple-min-0.0",
        "issue781-v2-lmcut-simple-min-0.2"],
    attributes=common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + ["time_for_pruning_operators"])

exp.run_steps()
