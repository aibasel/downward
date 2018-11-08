#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = []
CONFIGS = []
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(email="jendrik.seipp@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_algorithm(
    "base-ipdb-no-goal-vars", common_setup.get_repo_base(), "issue743-base",
    ['--search', 'astar(ipdb(max_time=900))'])
exp.add_algorithm(
    "v2-ipdb-no-goal-vars", common_setup.get_repo_base(), "issue743-v2",
    ['--search', 'astar(ipdb(max_time=900, use_co_effect_goal_variables=false))'])
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_absolute_report_step()
#exp.add_comparison_table_step()

exp.run_steps()
