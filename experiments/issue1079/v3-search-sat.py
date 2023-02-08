#! /usr/bin/env python3
# -*- coding: utf-8 -*-

from collections import defaultdict
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab import tools

from downward.reports.compare import ComparativeReport
from downward.reports import PlanningReport

import common_setup
from common_setup import IssueConfig, IssueExperiment


ARCHIVE_PATH = "ai/downward/issue1079"
EXPNAME = common_setup.get_experiment_name()
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REPO_DIR = os.environ["DOWNWARD_REPO"]
REVISIONS = ["issue1079-base", "issue1079-v3"]
CONFIGS = [
    IssueConfig("lama-first", [], driver_options=["--alias", "lama-first"]),
    IssueConfig("eager-ff", ["--search", "eager_greedy([ff()])"]),
    IssueConfig("eager-cg", ["--search", "eager_greedy([cg()])"]),
    IssueConfig("eager-cea", ["--search", "eager_greedy([cea()])"]),
]
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="gabriele.roeger@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

# This are the domains where we observed differences in the translator output
SUITE = [
    'freecell',
    'trucks-strips',
    'citycar-sat14-adl',
    'woodworking-sat08-strips',
    'woodworking-sat11-strips',
]

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    REPO_DIR,
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_comparison_table_step()
exp.add_scatter_plot_step(relative=True, attributes=["total_time"])
exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
