#! /usr/bin/env python
# -*- coding: utf-8 -*-


import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue995-base", "issue995-v1"]

CONFIGS = [
    IssueConfig("seq-opt-bjolp", [],
                driver_options=["--alias", "seq-opt-bjolp"]),
    IssueConfig("lm_exhaust", ["--evaluator", "lmc=lmcount(lm_exhaust(),admissible=true)",
                "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    IssueConfig("lm_hm", ["--evaluator", "lmc=lmcount(lm_hm(m=2),admissible=true)",
                "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    IssueConfig("seq-opt-bjolp-optimal", ["--evaluator", 
    "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true,optimal=true)",
    "--search", "astar(lmc,lazy_evaluator=lmc)"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="salome.eriksson@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"],
)

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
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
exp.add_parser(os.path.join(DIR, "landmark_parser.py"))


exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

ATTRIBUTES = [
    "cost",
    "coverage",
    "error",
    "evaluations",
    "expansions",
    "expansions_until_last_jump",
    "initial_h_value",
    "generated",
    "memory",
    "planner_memory",
    "planner_time",
    "quality",
    "run_dir",
    "score_evaluations",
    "score_expansions",
    "score_generated",
    "score_memory",
    "score_search_time",
    "score_total_time",
    "search_time",
    "total_time",
    "landmarks",
    "edges"
    ]
exp.add_comparison_table_step(attributes=ATTRIBUTES)
exp.add_scatter_plot_step(attributes=['search_time'])
exp.add_scatter_plot_step(attributes=['search_time'], relative=True)
exp.run_steps()

