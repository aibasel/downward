#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue548-base", "issue548-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_optimal_with_ipc11()

B_CONFIGS = {
    'rl-b50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))'],
    'cggl-b50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=cg_goal_level),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))'],
    'dfp-b50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))'],
}
G_CONFIGS = {
    'rl-ginf': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))'],
    'cggl-ginf': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=cg_goal_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))'],
    'dfp-ginf': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))'],
}
F_CONFIGS = {
    'rl-f50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_fh(max_states=50000),label_reduction=label_reduction(before_shrinking=false,before_merging=true)))'],
    'cggl-f50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=cg_goal_level),shrink_strategy=shrink_fh(max_states=50000),label_reduction=label_reduction(before_shrinking=false,before_merging=true)))'],
    'dfp-f50k': ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_fh(max_states=50000),label_reduction=label_reduction(before_shrinking=false,before_merging=true)))'],
}
CONFIGS = dict(B_CONFIGS)
CONFIGS.update(G_CONFIGS)
CONFIGS.update(F_CONFIGS)

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    test_suite=['depot:pfile1'],
    processes=4,
    email='silvan.sievers@unibas.ch',
    )
exp.add_comparison_table_step()

exp()
