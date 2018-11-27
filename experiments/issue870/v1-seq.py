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
BUILDS_AND_REVISIONS = [("release64", "issue870-base"), ("release64dynamic", "issue870-v1")]
CONFIG_NICKS = [
    ("seq", ["--search", "astar(operatorcounting([state_equation_constraints()]))"]),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=[],
    configs=[],
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
for build, rev in BUILDS_AND_REVISIONS:
    for config_nick, config in CONFIG_NICKS:
        exp.add_algorithm(
            ":".join([config_nick, build, rev]),
            common_setup.get_repo_base(),
            rev,
            config,
            build_options=[build],
            driver_options=["--build", build])

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_absolute_report_step()
#exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES

algorithm_pairs = [
    ("seq:release64:issue870-base",
     "seq:release64dynamic:issue870-v1",
     "Diff (seq)")
    ]
exp.add_report(
    ComparativeReport(algorithm_pairs, attributes=attributes),
    name="issue870-seq-static-vs-dynamic")

exp.run_steps()
