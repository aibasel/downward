#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue714-v1"]
CONFIGS = [
    IssueConfig(
        "-".join([search, h1, h2]),
        [
            "--heuristic", "h1={h1}".format(**locals()),
            "--heuristic", "h2={h2}".format(**locals()),
            "--search", "{search}([h1,h2], preferred=[h1,h2])".format(**locals())],
        driver_options=["--search-time-limit", "1m"])
    for h1, h2 in itertools.permutations(["cea", "cg", "ff"], 2)
    for search in ["lazy_greedy"]
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
exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
