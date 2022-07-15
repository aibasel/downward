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
            (f"{ISSUE}-base-lama-first", f"{ISSUE}-v3-lama-first"),
            (f"{ISSUE}-base-lama-first-pref", f"{ISSUE}-v3-lama-first-pref"),
            (f"{ISSUE}-base-lm-zg", f"{ISSUE}-v3-lm-zg"),
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
    common_setup.IssueConfig("lama-first", [],
                driver_options=["--alias", "lama-first"]),
    common_setup.IssueConfig(
        "lama-first-pref", ["--evaluator",
                            "hlm=lmcount(lm_factory=lm_reasonable_orders_hps("
                            "lm_rhw()),transform=adapt_costs(one),pref=true)",
                            "--evaluator", "hff=ff(transform=adapt_costs(one))",
                            "--search",
                            """lazy_greedy([hff,hlm],preferred=[hff,hlm], cost_type=one,reopen_closed=false)"""],
    ),
    common_setup.IssueConfig("lm-zg", ["--search", "eager_greedy([lmcount(lm_zg())])"],),
]

BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]

if common_setup.is_running_on_cluster():
    SUITE = common_setup.DEFAULT_SATISFICING_SUITE
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

exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
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

