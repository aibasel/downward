#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

EXPNAME = common_setup.get_experiment_name()
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue213-v8"]
BUILDS = ["debug64"]
CONFIG_DICT = {
    "eager_greedy_ff": [
        "--evaluator",
        "h=ff()",
        "--search",
        "eager_greedy([h], preferred=[h])"],
    "eager_greedy_cea": [
        "--evaluator",
        "h=cea()",
        "--search",
        "eager_greedy([h], preferred=[h])"],
    "lazy_greedy_add": [
        "--evaluator",
        "h=add()",
        "--search",
        "lazy_greedy([h], preferred=[h])"],
    "lazy_greedy_cg": [
        "--evaluator",
        "h=cg()",
        "--search",
        "lazy_greedy([h], preferred=[h])"],
    "lama-first": [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                   cost_type=one,reopen_closed=false)"""],
    "lama-first-typed": [
        "--evaluator",
        "hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=false)",
        "--evaluator", "hff=ff(transform=adapt_costs(one))",
        "--search",
            "lazy(alt([single(hff), single(hff, pref_only=true),"
            "single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000),"
            "preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=true,"
            "preferred_successors_first=false)"],
}
CONFIGS = [
    IssueConfig(
        "-".join([config_nick, build]),
        config,
        build_options=[build],
        driver_options=["--build", build, "--overall-time-limit", "5m"])
    for rev in REVISIONS
    for build in BUILDS
    for config_nick, config in CONFIG_DICT.items()
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
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

exp.add_absolute_report_step()
#exp.add_comparison_table_step()

exp.run_steps()
