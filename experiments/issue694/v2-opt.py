#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment, RelativeScatterPlotReport


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue694-v2-base", "issue694-v2"]
BUILDS = ["release32", "release64"]
SEARCHES = [
    ("blind", "astar(blind())"),
    ("lmcut", "astar(lmcut())"),
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        ["--search", search],
        build_options=[build],
        driver_options=["--build", build])
    for nick, search in SEARCHES
    for build in BUILDS
]
ATTRIBUTES = [
    "coverage", "error", "expansions_until_last_jump", "memory",
    "score_memory", "total_time", "score_total_time",
    "int_hash_set_load_factor", "int_hash_set_resizes"]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="jendrik.seipp@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_command('run-custom-parser', [os.path.join(DIR, 'custom-parser.py')])
exp.add_comparison_table_step(attributes=ATTRIBUTES)
for relative in [False, True]:
    exp.add_scatter_plot_step(relative=relative, attributes=["memory", "total_time"])

exp.run_steps()
