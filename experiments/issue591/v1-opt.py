#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue591-base", "issue591-v1"]
CONFIGS = [
    IssueConfig(heuristic, ["--search", "astar({})".format(heuristic)])
    for heuristic in [
        "blind()", "cegar(max_states=10000)", "hm()", "lmcut()", "hmax()"]
]
SUITE = [
    'barman-opt14-strips', 'cavediving-14-adl', 'childsnack-opt14-strips',
    'citycar-opt14-adl', 'floortile-opt14-strips', 'ged-opt14-strips',
    'hiking-opt14-strips', 'maintenance-opt14-adl',
    'openstacks-opt14-strips', 'parking-opt14-strips',
    'tetris-opt14-strips', 'tidybot-opt14-strips', 'transport-opt14-strips',
    'visitall-opt14-strips']
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
