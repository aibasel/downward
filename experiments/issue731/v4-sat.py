#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue731-base", "issue731-v4"]
BUILDS = ["release32"]
SEARCHES = [
    ("ff_lazy", ["--heuristic", "h=ff()", "--search", "lazy_greedy([h], preferred=[h])"]),
    ("cea_lazy", ["--heuristic", "h=cea()", "--search", "lazy_greedy([h], preferred=[h])"]),
    ("type_based", ["--heuristic", "h=ff()", "--search", "eager(alt([type_based([h, g()])]))"]),
    ("zhu_givan", [
        "--heuristic", "hlm=lmcount(lm_zg())",
        "--search", """lazy_greedy([hlm], preferred=[hlm])"""]),
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
ENVIRONMENT = BaselSlurmEnvironment(
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
exp.add_comparison_table_step()

exp.run_steps()
