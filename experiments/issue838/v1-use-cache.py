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
REVISIONS = ["issue838-v1"]
BUILDS = ["release32", "release64"]
CONFIG_NICKS = [
    ("lazy-greedy-cg-use-cache-{use_cache}".format(**locals()), [
        "--heuristic", "h=cg(use_cache={use_cache})".format(**locals()),
        "--search", "lazy_greedy([h],preferred=[h])"])
    for use_cache in [True, False]
]
CONFIGS = [
    IssueConfig(
        build + "-" + config_nick,
        config,
        build_options=[build],
        driver_options=["--build", build])
    for rev in REVISIONS
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
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
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

#exp.add_absolute_report_step()
#exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES
algorithm_pairs = [
    ("issue838-v1-{build}-lazy-greedy-cg-use-cache-False".format(**locals()),
     "issue838-v1-{build}-lazy-greedy-cg-use-cache-True".format(**locals()),
     "Diff ({build})".format(**locals()))
    for build in BUILDS]
exp.add_report(
    ComparativeReport(algorithm_pairs, attributes=attributes),
    name="{SCRIPT_NAME}".format(**locals()))

exp.run_steps()
