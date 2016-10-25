#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from downward import suites

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run

REVS = ["issue551-base", "issue551-v5"]
BENCHMARKS = os.path.expanduser('~/downward-benchmarks')
SUITE = suites.suite_satisficing()

CONFIGS = [
    IssueConfig("seq-sat-lama-2011", [], driver_options=["--alias", "seq-sat-lama-2011"]),
]

ENVIRONMENT = MaiaEnvironment(
    priority=0, email="manuel.heusner@unibas.ch")

if is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    environment=ENVIRONMENT
)

exp.add_suite(BENCHMARKS, SUITE)

exp.add_comparison_table_step()

exp()
