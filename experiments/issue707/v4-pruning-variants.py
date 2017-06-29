#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.reports import Attribute, geometric_mean

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

BENCHMARKS_DIR=os.path.expanduser('~/repos/downward/benchmarks')
REVISIONS = ["issue707-v4"]
CONFIGS = [
    IssueConfig('rl-b50k-nopruneunreachable', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_unreachable_states=false))']),
    IssueConfig('dfp-b50k-nopruneunreachable', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_unreachable_states=false))']),
    IssueConfig('sccs-dfp-b50k-nopruneunreachable', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_unreachable_states=false))']),
    IssueConfig('rl-ginf-nopruneunreachable', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_unreachable_states=false))']),
    IssueConfig('dfp-ginf-nopruneunreachable', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_unreachable_states=false))']),
    IssueConfig('sccs-dfp-ginf-nopruneunreachable', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=true),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_unreachable_states=false))']),
    IssueConfig('rl-f50k-nopruneunreachable', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=false))']),
    IssueConfig('dfp-f50k-nopruneunreachable', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=false))']),
    IssueConfig('sccs-dfp-f50k-nopruneunreachable', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_fh(),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,prune_unreachable_states=false))']),

    IssueConfig('rl-b50k-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_irrelevant_states=false))']),
    IssueConfig('dfp-b50k-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_irrelevant_states=false))']),
    IssueConfig('sccs-dfp-b50k-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_irrelevant_states=false))']),
    IssueConfig('rl-ginf-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_irrelevant_states=false))']),
    IssueConfig('dfp-ginf-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_irrelevant_states=false))']),
    IssueConfig('sccs-dfp-ginf-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=true),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_irrelevant_states=false))']),
    IssueConfig('rl-f50k-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_irrelevant_states=false))']),
    IssueConfig('dfp-f50k-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_irrelevant_states=false))']),
    IssueConfig('sccs-dfp-f50k-nopruneirrelevant', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_fh(),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,prune_irrelevant_states=false))']),

    IssueConfig('rl-b50k-noprune', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('dfp-b50k-noprune', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('sccs-dfp-b50k-noprune', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('rl-ginf-noprune', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('dfp-ginf-noprune', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('sccs-dfp-ginf-noprune', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=true),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('rl-f50k-noprune', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('dfp-f50k-noprune', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000,prune_unreachable_states=false,prune_irrelevant_states=false))']),
    IssueConfig('sccs-dfp-f50k-noprune', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_fh(),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,prune_unreachable_states=false,prune_irrelevant_states=false))']),
]
SUITE = DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email='silvan.sievers@unibas.ch')

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

exp.run_steps()
