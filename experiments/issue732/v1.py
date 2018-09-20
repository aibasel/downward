#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.reports import Attribute, arithmetic_mean, finite_sum, geometric_mean

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue732-base", "issue732-v1"]
CONFIGS = [
    IssueConfig(
        'astar-inf',
        ['--search', 'astar(const(infinity))'],
    ),
    IssueConfig(
        'astar-blind',
        ['--search', 'astar(blind())'],
    ),
    IssueConfig(
        'debug-astar-inf',
        ['--search', 'astar(const(infinity))'],
        build_options=["debug32"],
        driver_options=["--build=debug32"],
    ),
    IssueConfig(
        'debug-astar-blind',
        ['--search', 'astar(blind())'],
        build_options=["debug32"],
        driver_options=["--build=debug32"],
    ),
]
SUITE = list(sorted(set(common_setup.DEFAULT_OPTIMAL_SUITE) |
                    set(common_setup.DEFAULT_SATISFICING_SUITE)))
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="malte.helmert@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)

exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_resource('sg_parser', 'sg-parser.py', dest='sg-parser.py')
exp.add_command('sg-parser', ['{sg_parser}'])

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("sg_construction_time", functions=[finite_sum], min_wins=True),
    Attribute("sg_peak_mem_diff", functions=[finite_sum], min_wins=True),
    "error",
    "run_dir",
]

exp.add_absolute_report_step(attributes=attributes)

exp.run_steps()
