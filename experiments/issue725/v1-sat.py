#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue725-base", "issue725-v1"]
CONFIGS = [
    IssueConfig(
        "lama-first",
        [],
        driver_options=["--alias", "lama-first"]),
    IssueConfig(
        "lama",
        [],
        driver_options=["--alias", "seq-sat-lama-2011"]),
    IssueConfig("ehc_ff", ["--heuristic", "h=ff()", "--search", "ehc(h, preferred=h)"]),
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
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
