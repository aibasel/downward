#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue591-base", "issue591-v1"]
CONFIGS = [
    IssueConfig(
        "lazy_greedy_{}".format(heuristic),
        ["--heuristic", "h={}()".format(heuristic),
         "--search", "lazy_greedy(h, preferred=h)"])
    for heuristic in ["add", "cea", "cg", "ff"]
]
SUITE = [
    'barman-sat14-strips', 'cavediving-14-adl', 'childsnack-sat14-strips',
    'citycar-sat14-adl', 'floortile-sat14-strips', 'ged-sat14-strips',
    'hiking-sat14-strips', 'maintenance-sat14-adl',
    'openstacks-sat14-strips', 'parking-sat14-strips',
    'tetris-sat14-strips', 'thoughtful-sat14-strips',
    'transport-sat14-strips', 'visitall-sat14-strips']
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="jendrik.seipp@unibas.ch")

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
exp.add_scatter_plot_step(attributes=["total_time"])

exp()
