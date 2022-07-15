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
            (f"{ISSUE}-base-seq-opt-bjolp", f"{ISSUE}-v3-seq-opt-bjolp"),
            (f"{ISSUE}-base-lm-exhaust", f"{ISSUE}-v3-lm-exhaust"),
            (f"{ISSUE}-base-lm-hm2", f"{ISSUE}-v3-lm-hm2"),
            (f"{ISSUE}-base-seq-opt-bjolp-opt", f"{ISSUE}-v3-seq-opt-bjolp-opt"),
        ], attributes=ATTRIBUTES,
    )
    outfile = os.path.join(
        exp.eval_dir, "%s-compare.%s" % (exp.name, report.output_format)
    )
    report(exp.eval_dir, outfile)
    exp.add_report(report)


REVISIONS = [
    f"{ISSUE}-base",
    f"{ISSUE}-v3",
]

CONFIGS = [
    common_setup.IssueConfig("seq-opt-bjolp", [], driver_options=["--alias", "seq-opt-bjolp"]),
    common_setup.IssueConfig("lm-exhaust", ["--evaluator", "lmc=lmcount(lm_exhaust(),admissible=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    common_setup.IssueConfig("lm-hm2", ["--evaluator", "lmc=lmcount(lm_hm(m=2),admissible=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    common_setup.IssueConfig("seq-opt-bjolp-opt", ["--evaluator", "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true, optimal=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"]),
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

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser("landmark_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("lmgraph_generation_time"),
    Attribute("landmarks", min_wins=False),
    Attribute("disjunctive_landmarks", min_wins=False),
    Attribute("conjunctive_landmarks", min_wins=False),
    Attribute("orderings", min_wins=False),
]

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_step("comparison table", make_comparison_table)
exp.add_parse_again_step()

exp.run_steps()
