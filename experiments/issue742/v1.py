#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

BENCHMARKS_DIR=os.path.expanduser('~/repos/downward/benchmarks')
REVISIONS = ["issue742-v1"]
CONFIGS = [
    IssueConfig('rl-rnd-noprune', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_random,merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('rl-rnd-punr', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_random,merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=true,prune_irrelevant_states=false))']),
    IssueConfig('rl-rnd-pirr', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_random,merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=false,prune_irrelevant_states=true))']),
    IssueConfig('rl-rnd-fullprune', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_random,merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=true,prune_irrelevant_states=true))']),
]
SUITE = DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(email='silvan.sievers@unibas.ch')

if is_test_run():
    SUITE = ['depot:p01.pddl', 'depot:p02.pddl', 'parcprinter-opt11-strips:p01.pddl', 'parcprinter-opt11-strips:p02.pddl', 'mystery:prob07.pddl']
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_resource('ms_parser', 'ms-parser.py', dest='ms-parser.py')
exp.add_command('ms-parser', ['{ms_parser}'])
exp.add_suite(BENCHMARKS_DIR, SUITE)

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)

# m&s attributes
ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_atomic_construction_time = Attribute('ms_atomic_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,

    ms_construction_time,
    ms_atomic_construction_time,
    ms_abstraction_constructed,
    ms_final_size,
    ms_out_of_memory,
    ms_out_of_time,
    search_out_of_memory,
    search_out_of_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)
exp.add_scatter_plot_step()

exp.run_steps()
