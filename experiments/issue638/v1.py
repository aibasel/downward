#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue638-base", "issue638-v1"]
CONFIGS = [
    IssueConfig(heuristic, ["--search", "astar({})".format(heuristic)])
    for heuristic in [
        "cpdbs(patterns=systematic(3), dominance_pruning=true)",
        "cpdbs(patterns=systematic(4), dominance_pruning=true)",
        "operatorcounting([pho_constraints(patterns=systematic(3))])",
        "operatorcounting([pho_constraints(patterns=systematic(4))])",
    ]
]

sys.path.append(BENCHMARKS_DIR)
import suites

SUITE = suites.suite_optimal_strips()
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="florian.pommerening@unibas.ch")

if is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_command("parser", ["custom-parser.py"])

exp.add_comparison_table_step(
    attributes=exp.DEFAULT_TABLE_ATTRIBUTES +
               ["num_sga_patterns", "num_interesting_patterns"])
exp.add_scatter_plot_step(attributes=["total_time"])

exp()
