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
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue862-base", "issue862-v5"]
BUILDS = ["release32"]
CONFIG_DICT = {
    "lazy-greedy-{h}".format(**locals()): [
        "--evaluator",
        "h={h}()".format(**locals()),
        "--search",
        "lazy_greedy([h], preferred=[h])"]
    for h in ["hmax", "add", "ff", "cg", "cea"]
}
CONFIG_DICT["lama-first"] = [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                   cost_type=one,reopen_closed=false)"""]
CONFIG_DICT["blind"] = ["--search", "astar(blind())"]
CONFIGS = [
    IssueConfig(
        "-".join([config_nick, build]),
        config,
        build_options=[build],
        driver_options=["--build", build, "--overall-time-limit", "30m"])
    for build in BUILDS
    for config_nick, config in CONFIG_DICT.items()
]
SUITE = [
    "airport-adl",
    "assembly",
    "miconic-fulladl",
    "psr-large",
    "psr-middle",
]
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
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

#exp.add_absolute_report_step()
exp.add_comparison_table_step()

exp.run_steps()
