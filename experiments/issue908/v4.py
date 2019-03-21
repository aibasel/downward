#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue908-base", "issue908-v4"]
CONFIGS = [
    IssueConfig("cpdbs-hc", ['--search', 'astar(cpdbs(hillclimbing))']),
    IssueConfig("cpdbs-sys2", ['--search', 'astar(cpdbs(systematic(2)))']),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    email="silvan.sievers@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

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
exp.add_parser('parser.py')

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

attributes=exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend([
    Attribute('generator_computation_time', absolute=False, min_wins=True, functions=[geometric_mean]),
    Attribute('cpdbs_computation_time', absolute=False, min_wins=True, functions=[geometric_mean]),
    Attribute('dominance_pruning_time', absolute=False, min_wins=True, functions=[geometric_mean]),
])

#exp.add_absolute_report_step()
exp.add_comparison_table_step(attributes=attributes)
exp.add_scatter_plot_step(relative=True, attributes=['generator_computation_time', 'cpdbs_computation_time', 'dominance_pruning_time', "search_time", "total_time"])

exp.run_steps()
