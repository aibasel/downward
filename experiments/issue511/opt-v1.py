#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue511-base", "issue511-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()
CONFIGS = {
    "astar_blind": ["--search", "astar(blind())"],
    "astar_hmax": ["--search", "astar(hmax())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
