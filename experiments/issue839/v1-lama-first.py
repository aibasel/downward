#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue839-v1"]
BUILDS = ["release32"]
CONFIG_NICKS = [
    ("lama-first-syn", [
        "--heuristic",
        """hlm=lama_synergy(lm_rhw(reasonable_orders=true),
                                   transform=adapt_costs(one))""",
        "--heuristic", "hff=ff_synergy(hlm)",
        "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                   cost_type=one,reopen_closed=false)"""]),
    ("lama-first-no-syn-pref-false", [
            "--heuristic",
            "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true), transform=adapt_costs(one), pref=false)",
            "--heuristic", "hff=ff(transform=adapt_costs(one))",
            "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                       cost_type=one,reopen_closed=false)"""]),
    ("lama-first-no-syn-pref-true", [
            "--heuristic",
            "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true), transform=adapt_costs(one), pref=true)",
            "--heuristic", "hff=ff(transform=adapt_costs(one))",
            "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                       cost_type=one,reopen_closed=false)"""]),
]
CONFIGS = [
    IssueConfig(
        config_nick,
        config,
        build_options=[build],
        driver_options=["--build", build])
    for rev in REVISIONS
    for build in BUILDS
    for config_nick, config in CONFIG_NICKS
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
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
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

#exp.add_absolute_report_step()
#exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES


for build in BUILDS:
    algorithm_pairs = [
        ("{rev}-{nick1}".format(**locals()),
         "{rev}-{nick2}".format(**locals()),
         "Diff ({rev})".format(**locals()))
        for (nick1, _), (nick2, _) in itertools.combinations(CONFIG_NICKS, 2)]
    exp.add_report(
        ComparativeReport(algorithm_pairs, attributes=attributes),
        name="issue839-{nick1}-vs-{nick2}".format(**locals()))

exp.run_steps()
