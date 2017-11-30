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
REVISIONS = ["issue693-v5-base", "issue693-v5"]
BUILDS = ["release32"]
SEARCHES = [
    ("blind", "astar(blind())"),
    ("divpot", "astar(diverse_potentials())"),
    ("lmcut", "astar(lmcut())"),
    ("cegar", "astar(cegar())"),
    ("systematic2", "astar(cpdbs(systematic(2)))"),
    ("mas",
        "astar(merge_and_shrink("
        "shrink_strategy=shrink_bisimulation(greedy=false),"
        "merge_strategy=merge_stateless("
        "merge_selector=score_based_filtering("
        "scoring_functions=[goal_relevance,dfp,total_order])),"
        "label_reduction=exact(before_shrinking=true,before_merging=false),"
        "max_states=50000,threshold_before_merge=1))")
]
CONFIGS = [
    IssueConfig(
        "{nick}-{build}".format(**locals()),
        ["--search", search],
        build_options=[build],
        driver_options=["--build", build, "--search-time-limit", "5m"])
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
exp.add_comparison_table_step()

attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES

# Compare revisions.
for build in BUILDS:
    for rev1, rev2 in itertools.combinations(REVISIONS, 2):
        algorithm_pairs = [
            ("{rev1}-{config_nick}-{build}".format(**locals()),
             "{rev2}-{config_nick}-{build}".format(**locals()),
             "Diff ({config_nick}-{build})".format(**locals()))
            for config_nick, search in SEARCHES]
        exp.add_report(
            ComparativeReport(algorithm_pairs, attributes=attributes),
            name="issue693-opt-{rev1}-vs-{rev2}-{build}".format(**locals()))

exp.run_steps()
