#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute 

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

EXPNAME = common_setup.get_experiment_name()
DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue744-base"]
SEARCHES = [
    ("bjolp", [
        "--evaluator", "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true)",
        "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    ("blind", ["--search", "astar(blind())"]),
    ("cegar", ["--search", "astar(cegar())"]),
    ("divpot", ["--search", "astar(diverse_potentials())"]),
    ("ipdb", ["--search", "astar(ipdb())"]),
    ("lmcut", ["--search", "astar(lmcut())"]),
    ("mas", [
        "--search", "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),"
        " merge_strategy=merge_sccs(order_of_sccs=topological,"
        " merge_selector=score_based_filtering(scoring_functions=[goal_relevance, dfp, total_order])),"
        " label_reduction=exact(before_shrinking=true, before_merging=false),"
        " max_states=50000, threshold_before_merge=1))"]),
    ("seq+lmcut", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()]))"]),
    ("h2", ["--search", "astar(hm(m=2))"]),
    ("hmax", ["--search", "astar(hmax())"]),
]
CONFIGS = [
    IssueConfig(search_nick, search,
        driver_options=["--overall-time-limit", "30m"])
    for rev in REVISIONS
    for search_nick, search in SEARCHES
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="manuel.heusner@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser('custom-parser.py')

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

log_size = Attribute('log_size')
attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [log_size]

exp.add_absolute_report_step(attributes=attributes)
#exp.add_comparison_table_step()

sort_spec = [('log_size', 'desc')]
attributes = ['run_dir', 'log_size']
exp.add_sorted_report_step(attributes=attributes, sort_spec=sort_spec)

exp.run_steps()
