#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, finite_sum

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue732-base", "issue732-v6"]
BUILDS = ["debug32", "release32"]
CONFIGS = [
    IssueConfig(
        "lama-first-{build}".format(**locals()),
        [],
        build_options=[build],
        driver_options=["--alias", "lama-first", "--build", build])
    for build in BUILDS
]
SUITE = set(
    common_setup.DEFAULT_OPTIMAL_SUITE + common_setup.DEFAULT_SATISFICING_SUITE)
ENVIRONMENT = BaselSlurmEnvironment(
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
]

exp.add_comparison_table_step(attributes=attributes)

exp.run_steps()
