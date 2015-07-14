#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

import common_setup


REVS = ["issue548-base", "issue548-v2"]
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
exp.add_search_parser('ms-parser.py')

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)
proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)
actual_search_time = Attribute('actual_search_time', absolute=False, min_wins=True, functions=[gm])

# m&s attributes
ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[gm])
ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,
    proved_unsolvability,
    actual_search_time,

    ms_construction_time,
    ms_abstraction_constructed,
    ms_final_size,
    ms_out_of_memory,
    ms_out_of_time,
    search_out_of_memory,
    search_out_of_time,
    actual_search_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_comparison_table_step(attributes=attributes)

exp()
