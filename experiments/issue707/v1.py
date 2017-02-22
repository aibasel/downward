#! /usr/bin/env python
# -*- coding: utf-8 -*-

#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.reports import Attribute, geometric_mean

import common_setup
from common_setup import IssueConfig, IssueExperiment
try:
    from relativescatter import RelativeScatterPlotReport
    matplotlib = True
except ImportError:
    print 'matplotlib not availabe, scatter plots not available'
    matplotlib = False

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR=os.path.expanduser('~/repos/downward/benchmarks')
REVISIONS = ["issue707-base", "issue707-v1"]
CONFIGS = [
    IssueConfig('rl-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('dfp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('rl-ginf', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1))']),
    IssueConfig('dfp-ginf', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=infinity,threshold_before_merge=1))']),
    IssueConfig('rl-f50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000))']),
    IssueConfig('dfp-f50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_fh(),label_reduction=exact(before_shrinking=false,before_merging=true),max_states=50000))']),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email='silvan.sievers@unibas.ch')

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
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
proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)
actual_search_time = Attribute('actual_search_time', absolute=False, min_wins=True, functions=[geometric_mean])

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
    proved_unsolvability,
    actual_search_time,

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

exp.add_comparison_table_step(attributes=attributes)

#if matplotlib:
    #for attribute in ["total_time"]:
        #for config in CONFIGS:
            #exp.add_report(
                #RelativeScatterPlotReport(
                    #attributes=[attribute],
                    #filter_algorithm=["{}-{}".format(rev, config.nick) for rev in REVISIONS],
                    #get_category=lambda run1, run2: run1.get("domain"),
                #),
                #outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, *REVISIONS)
            #)

exp.run_steps()
