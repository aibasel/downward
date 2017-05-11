#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue714-base", "issue714-v1"]
CONFIGS = [
    IssueConfig(alias, [], driver_options=["--alias", alias])
    for alias in [
        "seq-sat-fdss-1", "seq-sat-fdss-2", "seq-sat-fdss-2014",
        "seq-sat-fd-autotune-1", "seq-sat-fd-autotune-2"]
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
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
exp.add_absolute_report_step(attributes=IssueExperiment.PORTFOLIO_ATTRIBUTES)
exp.add_comparison_table_step(attributes=IssueExperiment.PORTFOLIO_ATTRIBUTES)

exp.run_steps()
