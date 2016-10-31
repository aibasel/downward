#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run, get_repo_base
from relativescatter import RelativeScatterPlotReport


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REPO = get_repo_base()
SUITE = [
    'airport', 'barman-opt11-strips', 'barman-opt14-strips', 'blocks',
    'childsnack-opt14-strips', 'depot', 'driverlog',
    'elevators-opt08-strips', 'elevators-opt11-strips',
    'floortile-opt11-strips', 'floortile-opt14-strips', 'freecell',
    'ged-opt14-strips', 'grid', 'gripper', 'hiking-opt14-strips',
    'logistics00', 'logistics98', 'miconic', 'movie', 'mprime', 'mystery',
    'nomystery-opt11-strips', 'openstacks-opt08-strips',
    'openstacks-opt11-strips', 'openstacks-opt14-strips',
    'openstacks-strips', 'parcprinter-08-strips',
    'parcprinter-opt11-strips', 'parking-opt11-strips',
    'parking-opt14-strips', 'pathways-noneg', 'pegsol-08-strips',
    'pegsol-opt11-strips', 'pipesworld-notankage', 'pipesworld-tankage',
    'psr-small', 'rovers', 'satellite', 'scanalyzer-08-strips',
    'scanalyzer-opt11-strips', 'sokoban-opt08-strips',
    'sokoban-opt11-strips', 'storage', 'tetris-opt14-strips',
    'tidybot-opt11-strips', 'tidybot-opt14-strips', 'tpp',
    'transport-opt08-strips', 'transport-opt11-strips',
    'transport-opt14-strips', 'trucks-strips', 'visitall-opt11-strips',
    'visitall-opt14-strips', 'woodworking-opt08-strips',
    'woodworking-opt11-strips', 'zenotravel']
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="jendrik.seipp@unibas.ch")

if is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_algorithm(
    "01:issue657-v2-base:cegar",
    REPO,
    "issue657-v2-base",
    ["--search", "astar(cegar(max_states=10000,max_time=infinity))"])
exp.add_algorithm(
    "02:issue657-v2:cegar",
    REPO,
    "issue657-v2",
    ["--search", "astar(cegar(max_states=10000,max_time=infinity,max_transitions=infinity))"])

exp.add_absolute_report_step()
exp.add_report(RelativeScatterPlotReport(
        filter_config=["01:issue657-v2-base:cegar", "02:issue657-v2:cegar"],
        attributes=["total_time"],
        get_category=lambda run1, run2: run1["domain"],
        legend_location=(1.3, 0.5)),
    outfile="issue657-base-vs-v2.png")

exp()
