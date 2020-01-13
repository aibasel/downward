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
REVISIONS = ["issue744-v1"]
SEARCHES = [
    ("bjolp-silent", [
        "--evaluator", "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true)",
        "--search", "astar(lmc,lazy_evaluator=lmc, verbosity=silent)"]),
    ("blind-silent", ["--search", "astar(blind(), verbosity=silent)"]),
    ("cegar-silent", ["--search", "astar(cegar(), verbosity=silent)"]),
    # ("divpot", ["--search", "astar(diverse_potentials(), verbosity=silent)"]),
    ("ipdb-silent", ["--search", "astar(ipdb(), verbosity=silent)"]),
    ("lmcut-silent", ["--search", "astar(lmcut(), verbosity=silent)"]),
    ("mas-silent", [
        "--search", "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),"
        " merge_strategy=merge_sccs(order_of_sccs=topological,"
        " merge_selector=score_based_filtering(scoring_functions=[goal_relevance, dfp, total_order])),"
        " label_reduction=exact(before_shrinking=true, before_merging=false),"
        " max_states=50000, threshold_before_merge=1, verbosity=normal), verbosity=silent)"]),
    # ("seq+lmcut", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()]), verbosity=silent)"]),
    ("h2-silent", ["--search", "astar(hm(m=2), verbosity=silent)"]),
    ("hmax-silent", ["--search", "astar(hmax(), verbosity=silent)"]),

    ("bjolp-normal", [
        "--evaluator", "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true)",
        "--search", "astar(lmc,lazy_evaluator=lmc, verbosity=normal)"]),
    ("blind-normal", ["--search", "astar(blind(), verbosity=normal)"]),
    ("cegar-normal", ["--search", "astar(cegar(), verbosity=normal)"]),
    # ("divpot", ["--search", "astar(diverse_potentials(), verbosity=normal)"]),
    ("ipdb-normal", ["--search", "astar(ipdb(), verbosity=normal)"]),
    ("lmcut-normal", ["--search", "astar(lmcut(), verbosity=normal)"]),
    ("mas-normal", [
        "--search", "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),"
        " merge_strategy=merge_sccs(order_of_sccs=topological,"
        " merge_selector=score_based_filtering(scoring_functions=[goal_relevance, dfp, total_order])),"
        " label_reduction=exact(before_shrinking=true, before_merging=false),"
        " max_states=50000, threshold_before_merge=1, verbosity=normal), verbosity=normal)"]),
    # ("seq+lmcut", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()]), verbosity=normal)"]),
    ("h2-normal", ["--search", "astar(hm(m=2), verbosity=normal)"]),
    ("hmax-normal", ["--search", "astar(hmax(), verbosity=normal)"]),
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
    email="silvan.sievers@unibas.ch",
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
exp.add_parse_again_step()

log_size = Attribute('log_size')
attributes = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [log_size]

exp.add_absolute_report_step(attributes=attributes)
#exp.add_comparison_table_step()

sort_spec = [('log_size', 'desc')]
attributes = ['run_dir', 'log_size']
exp.add_sorted_report_step(attributes=attributes, sort_spec=sort_spec,filter_algorithm=[
    "{}-bjolp-silent".format(REVISIONS[0]),
    "{}-blind-silent".format(REVISIONS[0]),
    "{}-cegar-silent".format(REVISIONS[0]),
    "{}-ipdb-silent".format(REVISIONS[0]),
    "{}-lmcut-silent".format(REVISIONS[0]),
    "{}-mas-silent".format(REVISIONS[0]),
    "{}-h2-silent".format(REVISIONS[0]),
    "{}-hmax-silent".format(REVISIONS[0]),
],name="silent")
exp.add_sorted_report_step(attributes=attributes, sort_spec=sort_spec,filter_algorithm=[
    "{}-bjolp-normal".format(REVISIONS[0]),
    "{}-blind-normal".format(REVISIONS[0]),
    "{}-cegar-normal".format(REVISIONS[0]),
    "{}-ipdb-normal".format(REVISIONS[0]),
    "{}-lmcut-normal".format(REVISIONS[0]),
    "{}-mas-normal".format(REVISIONS[0]),
    "{}-h2-normal".format(REVISIONS[0]),
    "{}-hmax-normal".format(REVISIONS[0]),
],name="normal")

exp.run_steps()
