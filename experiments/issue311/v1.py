#! /usr/bin/env python
# -*- coding: utf-8 -*-

'''
Script to check correctness of eager_wastar.

Comparing eager_wastar with the equivalent version using
eager(single(w*h), reopen_closed=true).

Results should be the same for a given same value w.
'''

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue311"]
CONFIGS = [
    IssueConfig('eager_wastar_w1', ['--search', 'eager_wastar([lmcut], w=1)'], [], driver_options=['--overall-time-limit', '5m']),
    IssueConfig('eager_wastar_w2', ['--search', 'eager_wastar([lmcut], w=2)'], [], driver_options=['--overall-time-limit', '5m']),
    IssueConfig('eager_wastar_w5', ['--search', 'eager_wastar([lmcut], w=5)'], [], driver_options=['--overall-time-limit', '5m']),
    IssueConfig('eager_wastar_w100', ['--search', 'eager_wastar([lmcut], w=100)'], [], driver_options=['--overall-time-limit', '5m']),

    IssueConfig('eager_single_openlist_w1', ['--search', 'eager(single(sum([g(), weight(lmcut, 1)])), reopen_closed=true)'], [], driver_options=['--overall-time-limit', '5m']),
    IssueConfig('eager_single_openlist_w2', ['--search', 'eager(single(sum([g(), weight(lmcut, 2)])), reopen_closed=true)'], [], driver_options=['--overall-time-limit', '5m']),
    IssueConfig('eager_single_openlist_w5', ['--search', 'eager(single(sum([g(), weight(lmcut, 5)])), reopen_closed=true)'], [], driver_options=['--overall-time-limit', '5m']),
    IssueConfig('eager_single_openlist_w100', ['--search', 'eager(single(sum([g(), weight(lmcut, 100)])), reopen_closed=true)'], [], driver_options=['--overall-time-limit', '5m']),
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
