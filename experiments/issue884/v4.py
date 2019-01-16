#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
BUILD = "release64"
REVISIONS = ["issue884-v2", "issue884-v2-heap-queue", "issue884-v4"]
DRIVER_OPTIONS = ["--build", BUILD]
CONFIGS = [
    IssueConfig(
        nick + "-" + max_transitions_nick,
        config,
        build_options=[BUILD],
        driver_options=DRIVER_OPTIONS)
    for max_transitions_nick, max_transitions in [("2M", 2000000)]
    for nick, config in [
        ("cegar-original", ["--search", "astar(cegar(subtasks=[original()], max_transitions={max_transitions}))".format(**locals())]),
        ]
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = [
        #"depot:p02.pddl",
        "gripper:prob01.pddl"]
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
#exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(os.path.join(DIR, "parser.py"))

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

REFINEMENT_ATTRIBUTES = [
    "time_for_finding_traces",
    "time_for_finding_flaws",
    "time_for_splitting_states",
]
attributes = (
    IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
        "search_start_memory", "init_time", "time_analysis", "total_loops",
        "total_transitions"] +
    ["total_" + attr for attr in REFINEMENT_ATTRIBUTES])
exp.add_absolute_report_step(attributes=attributes)
exp.add_comparison_table_step(attributes=attributes)

for revisions in itertools.combinations(REVISIONS, 2):
    for attribute in ["total_time_for_finding_traces"]:
        for config in CONFIGS:
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_algorithm=["{}-{}".format(rev, config.nick) for rev in revisions],
                    get_category=lambda run1, run2: run1.get("domain")),
                outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, *revisions))

exp.run_steps()
