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
REVISIONS = ["issue870-base"]
BUILDS = ["release64", "release64dynamic"]
CONFIG_NICKS = [
    ("blind", ["--search", "astar(blind())"]),
    ("lmcut", ["--search", "astar(lmcut())"]),
    #("seq", ["--search", "astar(operatorcounting([state_equation_constraints()]))"]),
]
CONFIGS = [
    IssueConfig(
        config_nick + ":" + build,
        config,
        build_options=[build],
        driver_options=["--build", build])
    for rev in REVISIONS
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
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
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_parse_again_step()

#exp.add_absolute_report_step()
#exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES

for rev in REVISIONS:
    algorithm_pairs = [
        ("{rev}-{nick}:{build1}".format(**locals()),
         "{rev}-{nick}:{build2}".format(**locals()),
         "Diff ({rev}-{nick})".format(**locals()))
        for build1, build2 in itertools.combinations(BUILDS, 2)
        for nick, config in CONFIG_NICKS]
    exp.add_report(
        ComparativeReport(algorithm_pairs, attributes=attributes),
        name="issue839-opt-static-vs-dynamic")

exp.run_steps()
