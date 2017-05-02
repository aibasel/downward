#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue726-base", "issue726-v1"]
CONFIGS = [
    IssueConfig("blind", ["--search", "astar(blind())"]),
    IssueConfig("lmcut", ["--search", "astar(lmcut())"]),
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

exp.run_steps()
