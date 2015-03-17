#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue508-base", "issue508-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "mas1": ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true,group_by_h=false)))"],
    "mas2": ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,group_by_h=true)))"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
