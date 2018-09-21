#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue776-base", "issue776-v1"]
CONFIGS = [
    IssueConfig('lama-second', [
        "--heuristic",
        "hlm2=lama_synergy(lm_rhw(reasonable_orders=true,lm_cost_type=plusone),transform=adapt_costs(plusone))",
        "--heuristic",
        "hff2=ff_synergy(hlm2)",
        "--search",
        "lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],reopen_closed=false)"
    ]),
]

SUITE = [
    'barman-opt11-strips', 'barman-sat11-strips', 'citycar-opt14-adl',
    'citycar-sat14-adl', 'elevators-opt08-strips', 'elevators-opt11-strips',
    'elevators-sat08-strips', 'elevators-sat11-strips',
    'floortile-opt11-strips', 'floortile-opt14-strips',
    'floortile-sat11-strips', 'floortile-sat14-strips', 'openstacks-opt08-adl',
    'openstacks-sat08-adl', 'parcprinter-08-strips',
    'parcprinter-opt11-strips', 'parking-opt11-strips', 'parking-opt14-strips',
    'parking-sat11-strips', 'parking-sat14-strips', 'pegsol-08-strips',
    'pegsol-opt11-strips', 'pegsol-sat11-strips', 'scanalyzer-08-strips',
    'scanalyzer-opt11-strips', 'scanalyzer-sat11-strips',
    'sokoban-opt08-strips', 'sokoban-opt11-strips', 'sokoban-sat08-strips',
    'sokoban-sat11-strips', 'tetris-opt14-strips', 'tetris-sat14-strips',
    'woodworking-opt08-strips', 'woodworking-opt11-strips',
    'woodworking-sat08-strips', 'woodworking-sat11-strips'
]

ENVIRONMENT = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch",partition='infai_1')

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parse_again_step()

exp.add_comparison_table_step()

exp.run_steps()
