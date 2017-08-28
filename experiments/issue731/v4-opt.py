#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue731-base", "issue731-v4"]
BUILDS = ["release32"]
SEARCHES = [
    ("blind", "astar(blind())"),
    ("divpot", "astar(diverse_potentials())"),
    ("cegar", "astar(cegar())"),
    ("systematic2", "astar(cpdbs(systematic(2)))"),
    ("ipdb", "astar(ipdb())"),
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
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
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
exp.add_comparison_table_step()

exp.run_steps()
