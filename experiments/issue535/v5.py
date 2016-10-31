#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue535-base", "issue535-v5"]
CONFIGS = [
    IssueConfig(
        "{search}_ff".format(**locals()),
        ["--heuristic", "h=ff()",
         "--search", "{search}(h, preferred=h)".format(**locals())])
    for search in ["lazy_greedy", "eager_greedy", "ehc"]
] + [
    IssueConfig("lama-first", [], driver_options=["--alias", "lama-first"])
]
SUITE = [
    'airport', 'assembly', 'barman-sat11-strips', 'barman-sat14-strips',
    'blocks', 'cavediving-14-adl', 'childsnack-sat14-strips',
    'citycar-sat14-adl', 'depot', 'driverlog', 'elevators-sat08-strips',
    'elevators-sat11-strips', 'floortile-sat11-strips',
    'floortile-sat14-strips', 'freecell', 'ged-sat14-strips', 'grid',
    'gripper', 'hiking-sat14-strips', 'logistics00', 'logistics98',
    'maintenance-sat14-adl', 'miconic', 'miconic-fulladl',
    'miconic-simpleadl', 'movie', 'mprime', 'mystery',
    'nomystery-sat11-strips', 'openstacks', 'openstacks-sat08-adl',
    'openstacks-sat08-strips', 'openstacks-sat11-strips',
    'openstacks-sat14-strips', 'openstacks-strips', 'optical-telegraphs',
    'parcprinter-08-strips', 'parcprinter-sat11-strips',
    'parking-sat11-strips', 'parking-sat14-strips', 'pathways',
    'pathways-noneg', 'pegsol-08-strips', 'pegsol-sat11-strips',
    'philosophers', 'pipesworld-notankage', 'pipesworld-tankage',
    'psr-large', 'psr-middle', 'psr-small', 'rovers', 'satellite',
    'scanalyzer-08-strips', 'scanalyzer-sat11-strips', 'schedule',
    'sokoban-sat08-strips', 'sokoban-sat11-strips', 'storage',
    'tetris-sat14-strips', 'thoughtful-sat14-strips',
    'tidybot-sat11-strips', 'tpp', 'transport-sat08-strips',
    'transport-sat11-strips', 'transport-sat14-strips', 'trucks',
    'trucks-strips', 'visitall-sat11-strips', 'visitall-sat14-strips',
    'woodworking-sat08-strips', 'woodworking-sat11-strips', 'zenotravel']
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
exp.add_scatter_plot_step(relative=True, attributes=["total_time"])

exp()
