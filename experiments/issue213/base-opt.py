#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISION = "issue213-base"
BUILDS = ["release32", "release64"]
SEARCHES = [
    ("bjolp", "astar(lmcount(lm_merged([lm_rhw(), lm_hm(m=1)]), admissible=true), mpd=true)"),
    ("blind", "astar(blind())"),
    ("cegar", "astar(cegar())"),
    ("divpot", "astar(diverse_potentials())"),
    ("ipdb", "astar(ipdb(max_time=900))"),
    ("lmcut", "astar(lmcut())"),
    ("mas",
        "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false), "
        "merge_strategy=merge_dfp(), "
        "label_reduction=exact(before_shrinking=true, before_merging=false), max_states=100000, threshold_before_merge=1))"),
    ("seq", "astar(operatorcounting([state_equation_constraints()]))"),
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
    revisions=[REVISION],
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_absolute_report_step()

algorithm_pairs = []
for build1, build2 in itertools.combinations(BUILDS, 2):
    for config_nick, search in SEARCHES:
        algorithm_pairs.append(
            ("{REVISION}-{config_nick}-{build1}".format(**locals()),
             "{REVISION}-{config_nick}-{build2}".format(**locals()),
             "Diff ({})".format(config_nick)))
exp.add_report(
    ComparativeReport(
        algorithm_pairs,
        attributes=IssueExperiment.DEFAULT_TABLE_ATTRIBUTES),
    name="issue213-opt-comparison")
#exp.add_scatter_plot_step(attributes=["total_time", "memory"])

exp.run_steps()
