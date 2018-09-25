#! /usr/bin/env python
# -*- coding: utf-8 -*-

'''
Script to test possible eager version of LAMA

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
    IssueConfig('lama', [], driver_options=["--alias", "seq-sat-lama-2011"]),
    IssueConfig('eager_lama', [
        "--if-unit-cost",
        "--evaluator",
        "hlm=lama_synergy(lm_rhw(reasonable_orders=true))",
        "--evaluator", "hff=ff_synergy(hlm)",
        "--search", """iterated([
        lazy_greedy([hff,hlm],preferred=[hff,hlm]),
        eager_wastar([hff,hlm],preferred=[hff,hlm],w=5),
        eager_wastar([hff,hlm],preferred=[hff,hlm],w=3),
        eager_wastar([hff,hlm],preferred=[hff,hlm],w=2),
        eager_wastar([hff,hlm],preferred=[hff,hlm],w=1)
        ],repeat_last=true,continue_on_fail=true)""",
        "--if-non-unit-cost",
        "--evaluator",
        "hlm1=lama_synergy(lm_rhw(reasonable_orders=true,"
        "                           lm_cost_type=one),transform=adapt_costs(one))",
        "--evaluator", "hff1=ff_synergy(hlm1)",
        "--evaluator",
        "hlm2=lama_synergy(lm_rhw(reasonable_orders=true,"
        "                           lm_cost_type=plusone),transform=adapt_costs(plusone))",
        "--evaluator", "hff2=ff_synergy(hlm2)",
        "--search", """iterated([
        lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],
        cost_type=one,reopen_closed=false),
        lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],
        reopen_closed=false),
        eager_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),
        eager_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),
        eager_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),
        eager_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)
        ],repeat_last=true,continue_on_fail=true)""",
        "--always"
    ])
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
exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
