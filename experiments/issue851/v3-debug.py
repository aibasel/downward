#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from generalscatter import GeneralScatterPlotReport
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue851-v3"]
BUILDS = ["debug32"]
CONFIG_NICKS = [
    ('dfp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    # ('rl-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    # ('sbmiasm-b50k', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    # ('sccs-dfp-b50k', ['--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=["--build", build])
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="silvan.sievers@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser('ms-parser.py')

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)

# m&s attributes
ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_atomic_construction_time = Attribute('ms_atomic_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
ms_atomic_fts_constructed = Attribute('ms_atomic_fts_constructed', absolute=True, min_wins=False)
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
    ms_atomic_fts_constructed,
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
