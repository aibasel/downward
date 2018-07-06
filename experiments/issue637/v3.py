#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue637-v1", "issue637-v2", "issue637-v3"]
REVISIONS = ["issue637-v1", "issue637-v3"]
DRIVER_OPTIONS = ["--overall-time-limit", "30m"]
CONFIGS = [
    IssueConfig(
        "cegar-landmarks-goals",
        ["--search", "astar(cegar())"],
        driver_options=DRIVER_OPTIONS),
    IssueConfig(
        "cegar-original",
        ["--search", "astar(cegar(subtasks=[original()]))"],
        driver_options=DRIVER_OPTIONS),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
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
exp.add_parser(os.path.join(DIR, "parser.py"))

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')
exp.add_fetcher(os.path.join(DIR, 'data/issue637-v2-eval'))

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    "search_start_time", "search_start_memory", "init_time", "cartesian_states"]
exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)

if len(REVISIONS) == 2:
    for attribute in ["init_time", "expansions_until_last_jump"]:
        for config in CONFIGS:
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_algorithm=["{}-{}".format(rev, config.nick) for rev in REVISIONS],
                    get_category=lambda run1, run2: run1.get("domain")),
                outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, *REVISIONS))

exp.run_steps()
