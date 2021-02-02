#! /usr/bin/env python
# -*- coding: utf-8 -*-

import common_setup

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

REVISIONS = [
    "issue990-base",
    "issue990-v1",
]

CONFIGS = [
    common_setup.IssueConfig("seq-opt-bjolp", [],
                             driver_options=["--alias", "seq-opt-bjolp"]),
    common_setup.IssueConfig("lm-hm2", ["--evaluator", "lmc=lmcount(lm_hm(m=2),admissible=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    common_setup.IssueConfig("seq-opt-bjolp-opt", ["--evaluator", "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true, optimal=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"]),
    common_setup.IssueConfig("lm-exhaust", ["--evaluator", "lmc=lmcount(lm_exhaust(),admissible=true)", "--search", "astar(lmc,lazy_evaluator=lmc)"]),
]

BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REPO = os.environ["DOWNWARD_REPO"]

if common_setup.is_running_on_cluster():
    SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
    ENVIRONMENT = BaselSlurmEnvironment(
        partition="infai_2",
        email="tho.keller@unibas.ch",
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

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_parse_again_step()

exp.run_steps()

