#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue794-base", "issue794-v2"]
CONFIGS = [
    IssueConfig('blind', ['--search', 'astar(blind())']),
]
SUITE = [
    'assembly', 'miconic-fulladl', 'openstacks',
    'openstacks-sat08-adl', 'optical-telegraphs', 'philosophers',
    'psr-large', 'psr-middle', 'trucks',
]
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    email="florian.pommerening@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("axiom_time_parser.py")

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_comparison_table_step(attributes=exp.DEFAULT_TABLE_ATTRIBUTES + ["axiom_time_inner", "axiom_time_outer"])

for attribute in ["axiom_time_inner", "axiom_time_outer"]:
    for config in CONFIGS:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=[attribute],
                filter_algorithm=["{}-{}".format(rev, config.nick) for rev in REVISIONS],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, *REVISIONS)
        )

exp.run_steps()
