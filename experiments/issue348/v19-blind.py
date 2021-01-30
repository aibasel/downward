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
REVISIONS = ["issue348-base", "issue348-v19"]
CONFIGS = [
    IssueConfig("blind", ["--search", "astar(blind())"]),
]

ADL_DOMAINS = [
    "assembly",
    "miconic-fulladl",
    "openstacks",
    "openstacks-opt08-adl",
    "optical-telegraphs",
    "philosophers",
    "psr-large",
    "psr-middle",
    "trucks",
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE + ADL_DOMAINS
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE + ["openstacks-opt08-adl:p01.pddl"]
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

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

#exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step(relative=True, attributes=["total_time"], suffix="-strips", filter_domain=common_setup.DEFAULT_OPTIMAL_SUITE)
exp.add_scatter_plot_step(relative=True, attributes=["total_time"], suffix="-adl", filter_domain=ADL_DOMAINS)

exp.run_steps()
