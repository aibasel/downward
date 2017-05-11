#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue688-v3-base", "issue688-v3"]
BUILDS = ["release32"]
SEARCHES = [
    ("blind", ["--search", "astar(blind())"]),
    ("ipdb", ["--search", "astar(ipdb())"]),
    ("divpot", ["--search", "astar(diverse_potentials())"]),
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        search,
        build_options=[build],
        driver_options=["--build", build])
    for nick, search in SEARCHES
    for build in BUILDS
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="florian.pommerening@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.add_report(RelativeScatterPlotReport(
    attributes=["search_time"],
    filter_algorithm=["issue688-v3-base-blind-release32", "issue688-v3-blind-release32"],
    get_category=lambda run1, run2: run1.get("domain"),
), outfile="{}-blind-search_time.png".format(exp.name))

exp.run_steps()
