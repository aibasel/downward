#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue546-v1"]
LIMITS = {"search_time": 300, "search_memory": 1024}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "blind-lab-limits": ["--search", "astar(blind())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
