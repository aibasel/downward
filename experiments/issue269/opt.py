#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue269-base", "issue269-v1"]
LIMITS = {"search_time": 600}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "mas-label-order": ["--search", "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation,label_reduction_system_order=random))"],
    "mas-buckets": ["--search", "astar(merge_and_shrink(shrink_strategy=shrink_fh,label_reduction_system_order=regular))"],
    "gapdb": ["--search", "astar(gapdb())"],
    "ipdb": ["--search", "astar(ipdb())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
