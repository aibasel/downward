#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from downward import suites

from common_setup import IssueConfig, IssueExperiment, is_test_run


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue701-base", "issue701-v1"]
CONFIGS = [
    IssueConfig(
        alias, [], driver_options=["--alias", alias])
    for alias in [
        "seq-sat-fd-autotune-1", "seq-sat-fd-autotune-2",
        "seq-sat-lama-2011", "seq-sat-fdss-2014"]
]
SUITE = suites.suite_all()
ENVIRONMENT = MaiaEnvironment(
    priority=-10, email="manuel.heusner@unibas.ch")

if is_test_run():
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

exp()
