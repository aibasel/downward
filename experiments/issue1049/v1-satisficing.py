#! /usr/bin/env python
# -*- coding: utf-8 -*-

import common_setup

from common_setup import IssueConfig, IssueExperiment

import os

from lab.reports import Attribute

from downward.reports.absolute import AbsoluteReport

from lab.environments import LocalEnvironment, BaselSlurmEnvironment



REVISIONS = [
    "issue1049-v1",
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
    SUITE = common_setup.DEFAULT_SATISFICING_SUITE
    ENVIRONMENT = BaselSlurmEnvironment(
        partition="infai_2",
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

exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser("landmark_parser.py")
exp.add_parser("dead-end_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("disjunctive_landmarks", min_wins=False),
    Attribute("conjunctive_landmarks", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("dead-end_empty_first_achievers"),
    Attribute("dead-end_empty_possible_achievers"),
]

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_report(AbsoluteReport(attributes=ATTRIBUTES), outfile="issue1049-logging-report.html")
exp.add_parse_again_step()

exp.run_steps()

