#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment, RelativeScatterPlotReport


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue213-v1", "issue213-v3", "issue213-v4"]
BUILDS = ["release32", "release64"]
SEARCHES = [
    ("blind", "astar(blind())"),
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        ["--search", search],
        build_options=[build],
        driver_options=["--build", build])
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
exp.add_absolute_report_step()

attributes = [
    "coverage", "error", "expansions_until_last_jump", "memory",
    "score_memory", "total_time", "score_total_time",
    "hash_set_load_factor", "hash_set_resizings"]

# Compare revisions.
# lmcut-base-32 vs. lmcut-v1-32 vs. lmcut-v3-32
# lmcut-base-64 vs. lmcut-v1-64 vs. lmcut-v3-64
for build in BUILDS:
    for rev1, rev2 in itertools.combinations(REVISIONS, 2):
        algorithm_pairs = [
            ("{rev1}-{config_nick}-{build}".format(**locals()),
             "{rev2}-{config_nick}-{build}".format(**locals()),
             "Diff ({config_nick}-{build})".format(**locals()))
            for config_nick, search in SEARCHES]
        exp.add_report(
            ComparativeReport(algorithm_pairs, attributes=attributes),
            name="issue213-{rev1}-vs-{rev2}-{build}".format(**locals()))

# Compare builds.
# lmcut-base-32 vs. lmcut-base-64
# lmcut-v1-32 vs. lmcut-v1-64
# lmcut-v3-32 vs. lmcut v3-64
for build1, build2 in itertools.combinations(BUILDS, 2):
    for rev in REVISIONS:
        algorithm_pairs = [
            ("{rev}-{config_nick}-{build1}".format(**locals()),
             "{rev}-{config_nick}-{build2}".format(**locals()),
             "Diff ({config_nick}-{rev})".format(**locals()))
            for config_nick, search in SEARCHES]
        exp.add_report(
            ComparativeReport(algorithm_pairs, attributes=attributes),
            name="issue213-{build1}-vs-{build2}-{rev}".format(**locals()))

for attribute in ["total_time", "memory"]:
    exp.add_report(
        RelativeScatterPlotReport(
            attributes=[attribute],
            filter_algorithm=["issue213-v1-blind-release64", "issue213-v4-blind-release64"]),
        name="issue213-relative-scatter-blind-m64-v1-vs-v4-{}".format(attribute))

exp.run_steps()
