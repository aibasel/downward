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
REVISIONS = ["issue213-v7"]
BUILDS = ["release32", "release64"]
CONFIG_DICT = {
    "eager_greedy_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "eager_greedy([h], preferred=[h])"],
    "eager_greedy_cea": [
        "--heuristic",
        "h=cea()",
        "--search",
        "eager_greedy([h], preferred=[h])"],
    "lazy_greedy_add": [
        "--heuristic",
        "h=add()",
        "--search",
        "lazy_greedy([h], preferred=[h])"],
    "lazy_greedy_cg": [
        "--heuristic",
        "h=cg()",
        "--search",
        "lazy_greedy([h], preferred=[h])"],
    "lama-first": [
        "--heuristic",
        """hlm=lama_synergy(lm_rhw(reasonable_orders=true,lm_cost_type=one),
                                   transform=adapt_costs(one))""",
        "--heuristic", "hff=ff_synergy(hlm)",
        "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                   cost_type=one,reopen_closed=false)"""],
    "ff-typed": [
        "--heuristic", "hff=ff()",
        "--search",
            "lazy(alt([single(hff), single(hff, pref_only=true),"
            " type_based([hff, g()])], boost=1000),"
            " preferred=[hff], cost_type=one)"],
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

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

#exp.add_absolute_report_step()
#exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES

# Compare builds.
for build1, build2 in itertools.combinations(BUILDS, 2):
    algorithm_pairs = [
        ("{rev}-{config_nick}-{build1}".format(**locals()),
         "{rev}-{config_nick}-{build2}".format(**locals()),
         "Diff ({config_nick}-{rev})".format(**locals()))
        for config_nick, _ in CONFIG_DICT.items()]
    exp.add_report(
        ComparativeReport(algorithm_pairs, attributes=attributes),
        name="issue213-{build1}-vs-{build2}-{SCRIPT_NAME}".format(**locals()))

exp.run_steps()
