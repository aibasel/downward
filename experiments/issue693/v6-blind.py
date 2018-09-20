#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment, RelativeScatterPlotReport


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue693-v5", "issue693-v6"]
BUILDS = ["release32", "release64"]
SEARCHES = [
    ("blind", "astar(blind())"),
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        ["--search", search],
        build_options=[build],
        driver_options=["--build", build, "--search-time-limit", "1m"])
    for nick, search in SEARCHES
    for build in BUILDS
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="jendrik.seipp@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

# Compare revisions.
for build in BUILDS:
    for rev1, rev2 in itertools.combinations(REVISIONS, 2):
        algorithm_pairs = [
            ("{rev1}-{config_nick}-{build}".format(**locals()),
             "{rev2}-{config_nick}-{build}".format(**locals()),
             "Diff ({config_nick}-{build})".format(**locals()))
            for config_nick, search in SEARCHES]
        exp.add_report(
            ComparativeReport(
                algorithm_pairs,
                attributes=IssueExperiment.DEFAULT_TABLE_ATTRIBUTES),
            name="issue693-opt-{rev1}-vs-{rev2}-{build}".format(**locals()))

        for config_nick, search in SEARCHES:
            algorithms = [
                "{rev1}-{config_nick}-{build}".format(**locals()),
                "{rev2}-{config_nick}-{build}".format(**locals())]
            for attribute in ["total_time", "memory"]:
                exp.add_report(
                    RelativeScatterPlotReport(
                        attributes=[attribute],
                        filter_algorithm=algorithms,
                        get_category=lambda run1, run2: run1["domain"]),
                    name="issue693-relative-scatter-{config_nick}-{build}-{rev1}-vs-{rev2}-{attribute}".format(**locals()))

exp.run_steps()
