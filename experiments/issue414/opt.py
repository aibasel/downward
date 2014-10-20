#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from downward import suites

import common_setup


DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))


REVS = ["issue414-base", "issue414"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

# The aliases are adjusted for the respective driver scripts by lab.
CONFIGS = {
    "ipdb": ["--search", "astar(ipdb())"],
}
for alias in ["seq-opt-bjolp", "seq-opt-fdss-1", "seq-opt-fdss-2",
              "seq-opt-lmcut", "seq-opt-merge-and-shrink"]:
    CONFIGS[alias] = ["--alias", alias]

exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_comparison_table_step()

exp()
