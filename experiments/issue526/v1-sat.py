#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue526-base", "issue526-v1"]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    email="manuel.heusner@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

CONFIGS = [
    IssueConfig(
        "ehc_ff", 
        ["--heuristic", "h=ff()", "--search", "ehc(h, preferred=[h])"],
        driver_options=["--overall-time-limit", "5m"]),
    IssueConfig(
        "lama-first-lazy",
        [],
        driver_options=["--alias", "lama-first", "--overall-time-limit", "5m"]),
    IssueConfig(
        "lama-first-eager",
        ["--evaluator", 
         """hlm=lama_synergy(lm_rhw(reasonable_orders=true,lm_cost_type=one),
                             transform=adapt_costs(one))""",
         "--evaluator", 
         "hff=ff_synergy(hlm)", 
         "--search", 
         """eager_greedy([hff,hlm],preferred=[hff,hlm],
                        cost_type=one)"""],
        driver_options=["--overall-time-limit", "5m"]),
]


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

#exp.add_absolute_report_step()
exp.add_comparison_table_step()

for attribute in ["memory", "total_time"]:
    for config in CONFIGS:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=[attribute],
                filter_algorithm=["{}-{}".format(rev, config.nick) for rev in REVISIONS],
                get_category=lambda run1, run2: run1.get("domain")),
            outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, *REVISIONS))

exp.run_steps()
