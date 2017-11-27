#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, MaiaEnvironment

from downward.reports import compare

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = MaiaEnvironment(
    priority=0, email="jendrik.seipp@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=[],
    configs=[],
    environment=ENVIRONMENT,
)

compared_algorithms = []
for search in ["eager_greedy", "lazy_greedy"]:
    for h1, h2 in itertools.permutations(["cea", "cg", "ff"], 2):
        rev = "issue714-base"
        config_nick = "-".join([search, h1, h2])
        algo1 = common_setup.get_algo_nick(rev, config_nick)
        exp.add_algorithm(
            algo1,
            common_setup.get_repo_base(),
            rev,
            [
                "--heuristic", "h{h1}={h1}".format(**locals()),
                "--heuristic", "h{h2}={h2}".format(**locals()),
                "--search", "{search}(h{h1}, h{h2}, preferred=[h{h1},h{h2}])".format(**locals())],
            driver_options=["--search-time-limit", "1m"])

        rev = "issue714-v1"
        config_nick = "-".join([search, h1, h2])
        algo2 = common_setup.get_algo_nick(rev, config_nick)
        exp.add_algorithm(
            algo2,
            common_setup.get_repo_base(),
            rev,
            [
                "--heuristic", "h{h1}={h1}".format(**locals()),
                "--heuristic", "h{h2}={h2}".format(**locals()),
                "--search", "{search}([h{h1},h{h2}], preferred=[h{h1},h{h2}])".format(**locals())],
            driver_options=["--search-time-limit", "1m"])

        compared_algorithms.append([algo1, algo2, "Diff ({config_nick})".format(**locals())])

exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_absolute_report_step()
exp.add_report(compare.ComparativeReport(
        compared_algorithms,
        attributes=IssueExperiment.DEFAULT_TABLE_ATTRIBUTES),
    name=common_setup.get_experiment_name() + "-comparison")

exp.run_steps()
