#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue540-base", "issue540-v1"]
LIMITS = {"search_time": 300}
SUITE = suites.suite_optimal_with_ipc11()
CONFIGS = {
    "blind": ["--search", "astar(blind())"],
    "ipdb": ["--search", "astar(ipdb(max_time=150))"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
