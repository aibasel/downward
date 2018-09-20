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
REVISIONS = ["issue693-v4-base", "issue693-v4"]
BUILDS = ["release32", "release64"]
SEARCHES = [
    ("ff_lazy", ["--heuristic", "h=ff()", "--search", "lazy_greedy(h, preferred=h)"]),
    ("add_lazy", ["--heuristic", "h=add()", "--search", "lazy_greedy(h, preferred=h)"]),
    ("ff_eager", ["--heuristic", "h=ff()", "--search", "eager_greedy(h, preferred=h)"]),
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        search,
        build_options=[build],
        driver_options=["--build", build])
    for nick, search in SEARCHES
    for build in BUILDS
] + [
    IssueConfig(
        "lama-first-{build}".format(**locals()),
        [],
        build_options=[build],
        driver_options=["--build", build, "--alias", "lama-first"])
    for build in BUILDS
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
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
exp.add_absolute_report_step()
exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES

# Compare revisions.
for build in BUILDS:
    for rev1, rev2 in itertools.combinations(REVISIONS, 2):
        algorithm_pairs = [
            ("{rev1}-{config_nick}-{build}".format(**locals()),
             "{rev2}-{config_nick}-{build}".format(**locals()),
             "Diff ({config_nick}-{build})".format(**locals()))
            for config_nick in [nick for nick, _ in SEARCHES] + ["lama-first"]]
        exp.add_report(
            ComparativeReport(algorithm_pairs, attributes=attributes),
            name="issue693-sat-{rev1}-vs-{rev2}-{build}".format(**locals()))

exp.run_steps()
