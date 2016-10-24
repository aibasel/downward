#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from downward import suites

from lab.environments import LocalEnvironment, MaiaEnvironment

from common_setup import IssueConfig, IssueExperiment, is_test_run

REVS = ["issue551-base", "issue551-v4"]
BENCHMARKS = os.path.expanduser('~/downward-benchmarks')
SUITE = suites.suite_optimal()

CONFIGS = [
    IssueConfig("seq-opt-bjolp", [], driver_options=["--alias", "seq-opt-bjolp"]),
    IssueConfig("seq-opt-bjolp-ocp", [
        "--landmarks", "lm=lm_merged([lm_rhw(),lm_hm(m=1)])",
        "--heuristic", "hlm=lmcount(lm,admissible=true,optimal=true)",
        "--search",    "astar(hlm,mpd=true)"]),
]

ENVIRONMENT = MaiaEnvironment(
    priority=0, email="manuel.heusner@unibas.ch")

if is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    environment=ENVIRONMENT
)

exp.add_suite(BENCHMARKS, SUITE)

exp.add_comparison_table_step()
exp.add_comparison_table_step(attributes=["memory","total_time", "search_time", "landmarks_generation_time"])
exp.add_scatter_plot_step(relative=True, attributes=["memory","total_time", "search_time", "landmarks_generation_time"])

exp()
