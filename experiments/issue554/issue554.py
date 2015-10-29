#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

REVS = ["issue554-base", "issue554-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "astar_hmax": ["--search", "astar(hmax())"],
    "gbfs_gc": ["--search", "eager_greedy(goalcount())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()
exp()
