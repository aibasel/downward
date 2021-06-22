#! /usr/bin/env python
# -*- coding: utf-8 -*-


import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment


def make_comparison_table():
    report = common_setup.ComparativeReport(
        algorithm_pairs=[
            ("issue999-base-seq-opt-bjolp", "issue999-v1-seq-opt-bjolp"),
        ], attributes=IssueExperiment.DEFAULT_TABLE_ATTRIBUTES,
    )
    outfile = os.path.join(
        exp.eval_dir, "%s-compare.%s" % (exp.name, report.output_format)
    )
    report(exp.eval_dir, outfile)

    exp.add_report(report)


DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue999-base", "issue999-v1"]

CONFIGS = [
    IssueConfig("seq-opt-bjolp", [],
                driver_options=["--alias", "seq-opt-bjolp"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="tho.keller@unibas.ch",
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

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_step("comparison table", make_comparison_table)

exp.run_steps()

