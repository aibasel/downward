#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, MaiaEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue688-v3-base", "issue688-v3"]
BUILDS = ["release32"]
SEARCHES = [
    ("eager_ff", ["--heuristic", "h=ff()", "--search", "eager_greedy(h, preferred=h)"]),
    ("lazy_add", ["--heuristic", "h=add()", "--search", "lazy_greedy(h, preferred=h)"]),
    ("ehc_ff", ["--heuristic", "h=ff()", "--search", "ehc(h, preferred=h)"]),
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
]
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="florian.pommerening@unibas.ch")

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

exp.run_steps()
