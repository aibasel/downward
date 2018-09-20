#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue635-base", "issue635-v2"]
CONFIGS = [
    IssueConfig(
        heuristic,
        ["--search", "astar({})".format(heuristic)],
        driver_options=["--search-time-limit", "10m"])
    for heuristic in ["hm(m=2)"]
]
SUITE = [
    'airport', 'barman-opt14-strips', 'blocks',
    'childsnack-opt14-strips', 'depot', 'driverlog',
    'elevators-opt11-strips', 'floortile-opt14-strips', 'freecell',
    'ged-opt14-strips', 'grid', 'gripper', 'hiking-opt14-strips',
    'logistics00', 'logistics98', 'miconic', 'movie', 'mprime', 'mystery',
    'nomystery-opt11-strips', 'openstacks-opt14-strips',
    'openstacks-strips', 'parcprinter-opt11-strips',
    'parking-opt14-strips', 'pathways-noneg', 'pegsol-opt11-strips',
    'pipesworld-notankage', 'pipesworld-tankage', 'psr-small', 'rovers',
    'satellite', 'scanalyzer-opt11-strips', 'sokoban-opt11-strips',
    'storage', 'tetris-opt14-strips', 'tidybot-opt14-strips', 'tpp',
    'transport-opt14-strips', 'trucks-strips', 'visitall-opt14-strips',
    'woodworking-opt11-strips', 'zenotravel']
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
exp.add_scatter_plot_step(relative=True, attributes=["memory", "total_time"])

exp()
