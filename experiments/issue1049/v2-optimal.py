#! /usr/bin/env python
# -*- coding: utf-8 -*-

import common_setup
from common_setup import IssueConfig, IssueExperiment

import os

from lab.reports import Attribute

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

ISSUE = "issue1049"

def make_comparison_table():
    report = common_setup.ComparativeReport(
        algorithm_pairs=[
            (f"{ISSUE}-v1-seq-opt-bjolp", f"{ISSUE}-v2-seq-opt-bjolp"),
            (f"{ISSUE}-v1-lm-exhaust", f"{ISSUE}-v2-lm-exhaust"),
            (f"{ISSUE}-v1-lm-hm2", f"{ISSUE}-v2-lm-hm2"),
            (f"{ISSUE}-v1-seq-opt-bjolp-opt", f"{ISSUE}-v2-seq-opt-bjolp-opt"),
        ], attributes=ATTRIBUTES, filter=remove_unfinished_tasks
    )
    outfile = os.path.join(
        exp.eval_dir, "%s-compare.%s" % (exp.name, report.output_format)
    )
    report(exp.eval_dir, outfile)
    exp.add_report(report)


REVISIONS = [
    f"{ISSUE}-v1",
    f"{ISSUE}-v2",
]

DRIVER_OPTIONS = ["--overall-time-limit", "5m"]

CONFIGS = [
    common_setup.IssueConfig("seq-opt-bjolp", [], driver_options=["--alias", "seq-opt-bjolp"]+DRIVER_OPTIONS),
    common_setup.IssueConfig("lm-exhaust", ["--evaluator", "lmc=lmcount(lm_exhaust(),admissible=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"], driver_options=DRIVER_OPTIONS),
    common_setup.IssueConfig("lm-hm2", ["--evaluator", "lmc=lmcount(lm_hm(m=2),admissible=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"], driver_options=DRIVER_OPTIONS),
    common_setup.IssueConfig("seq-opt-bjolp-opt", ["--evaluator", "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true, optimal=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"], driver_options=DRIVER_OPTIONS),
]

BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]

if common_setup.is_running_on_cluster():
    SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
    ENVIRONMENT = BaselSlurmEnvironment(
        partition="infai_1",
        email="remo.christen@unibas.ch",
        export=["PATH", "DOWNWARD_BENCHMARKS"],
    )
else:
    SUITE = common_setup.IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=2)

exp = common_setup.IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)

def remove_unfinished_tasks(run):
    if "expansions" in run:
        run["dead-end_empty_possible_achievers_finished"] = run["dead-end_empty_possible_achievers"]
    return True

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser("landmark_parser.py")
exp.add_parser("dead-end_parser-v2.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("disjunctive_landmarks", min_wins=False),
    Attribute("conjunctive_landmarks", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("dead-end_empty_first_achievers", min_wins=False),
    Attribute("dead-end_empty_possible_achievers", min_wins=False),
    Attribute("dead-end_empty_possible_achievers_finished", min_wins=False),
]

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_step("comparison table", make_comparison_table)
exp.add_parse_again_step()

exp.run_steps()
