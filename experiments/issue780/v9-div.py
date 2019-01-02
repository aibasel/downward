#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue780-v7", "issue780-v8", "issue780-v9"]
ABSTRACTIONS = ("["
    "projections(hillclimbing(max_time=1, random_seed=0)), "
    "projections(systematic(2)), "
    "cartesian()]")
CONFIGS = [
    IssueConfig(
        "scp-sys2-cart-200s",
        ["--search", "astar(saturated_cost_partitioning({ABSTRACTIONS}, diversify=true, max_orders=infinity, max_time=2, max_optimization_time=0))".format(**locals())],
        build_options=["release64"],
        driver_options=["--build", "release64"]),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])
ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    "search_start_time", "search_start_memory",
    "cost_partitionings"]

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

exp.add_absolute_report_step(attributes=ATTRIBUTES)
exp.add_comparison_table_step(attributes=ATTRIBUTES)
exp.add_scatter_plot_step(
    relative=True,
    attributes=["cost_partitionings", "search_start_memory"])

exp.run_steps()
