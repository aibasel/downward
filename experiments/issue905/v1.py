#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue905-base", "issue905-v1"]
CONFIGS = [
    IssueConfig("cegar-original-1M", ["--search", "astar(cegar(subtasks=[original()], max_transitions=1M, max_time=infinity))"]),
    IssueConfig("cegar-lm-goals-1M", ["--search", "astar(cegar(subtasks=[landmarks(), goals()], max_transitions=1M, max_time=infinity))"]),
    IssueConfig("cegar-original-900s", ["--search", "astar(cegar(subtasks=[original()], max_transitions=infinity, max_time=900))"]),
    IssueConfig("cegar-lm-goals-900s", ["--search", "astar(cegar(subtasks=[landmarks(), goals()], max_transitions=infinity, max_time=900))"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    email="jendrik.seipp@unibas.ch",
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
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(os.path.join(DIR, "parser.py"))

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

REFINEMENT_ATTRIBUTES = [
    "time_for_finding_traces",
    "time_for_finding_flaws",
    "time_for_splitting_states",
]
attributes = (
    IssueExperiment.DEFAULT_TABLE_ATTRIBUTES +
    ["search_start_memory", "init_time", "time_analysis"] +
    ["total_" + attr for attr in REFINEMENT_ATTRIBUTES])
#exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)
exp.add_scatter_plot_step(relative=True, attributes=["init_time", "search_time", "total_time"])

exp.run_steps()
