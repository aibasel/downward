#! /usr/bin/env python
# -*- coding: utf-8 -*-


import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

import common_setup
from common_setup import IssueConfig, IssueExperiment

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("landmarks_disjunctive", min_wins=False),
    Attribute("landmarks_conjunctive", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("lmgraph_generation_time"),
]

def make_comparison_table():
    report = common_setup.ComparativeReport(
        algorithm_pairs=[
            ("issue999-base-seq-opt-bjolp", "issue999-v5-seq-opt-bjolp"),
            ("issue999-base-lm_exhaust", "issue999-v5-lm_exhaust"),
            ("issue999-base-lm_hm", "issue999-v5-lm_hm"),
            ("issue999-base-seq-opt-bjolp-optimal", "issue999-v5-seq-opt-bjolp-optimal"),
        ], attributes=ATTRIBUTES,
    )
    outfile = os.path.join(
        exp.eval_dir, "%s-compare.%s" % (exp.name, report.output_format)
    )
    report(exp.eval_dir, outfile)

    exp.add_report(report)


DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue999-base", "issue999-v5"]

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
    email="clemens.buechner@unibas.ch",
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
exp.add_parser("landmark_parser.py")

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_step("comparison table", make_comparison_table)
#exp.add_parse_again_step()

exp.run_steps()

