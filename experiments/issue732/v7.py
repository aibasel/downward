#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, finite_sum

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    "issue732-{rev}".format(**locals())
    for rev in ["base", "v1", "v2", "v3", "v4", "v5", "v6", "v7"]]
BUILDS = ["release32"]
SEARCHES = [
    ("astar-inf", ["--search", "astar(const(infinity))"]),
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        config,
        build_options=[build],
        driver_options=["--build", build])
    for nick, config in SEARCHES
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

# Instead of comparing all revision pairs in separate reports, create a
# single report comparing neighboring revisions.
# exp.add_comparison_table_step(attributes=attributes)
compared_configs = []
for rev1, rev2 in zip(REVISIONS[:-1], REVISIONS[1:]):
    for config in CONFIGS:
        config_nick = config.nick
        compared_configs.append(
            ("{rev1}-{config_nick}".format(**locals()),
             "{rev2}-{config_nick}".format(**locals()),
             "Diff ({config_nick})".format(**locals())))
exp.add_report(
    ComparativeReport(compared_configs, attributes=attributes),
    name="compare-all-tags")

exp.run_steps()
