#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup
import configs


REVS = ["issue508-base", "issue508-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

configs_optimal_core = configs.default_configs_optimal(ipc=False)
CONFIGS = {}
for nick in ["astar_merge_and_shrink_bisim", "astar_merge_and_shrink_greedy_bisim"]:
    CONFIGS[nick] = configs_optimal_core[nick]

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
