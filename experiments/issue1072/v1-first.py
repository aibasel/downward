#! /usr/bin/env python3

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

import common_setup
from common_setup import IssueConfig, IssueExperiment

ARCHIVE_PATH = "ai/downward/issue1075"
DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = [
    "issue1072-v1",
]

CONFIGS = []

for use_preferrence_hierarchy in ["true", "false"]:
    for use_preferred_operators_conjunctive in ["true", "false"]:
        CONFIGS += [
            IssueConfig(
                f"lama-hm2-first-hierarchy={use_preferrence_hierarchy}-conjunctive={use_preferred_operators_conjunctive}",
                ["--evaluator",
                 "hlm=lmcount(lm_factory=lm_reasonable_orders_hps("
                 f"lm_hm(m=2)),transform=adapt_costs(one),pref=true, use_preferrence_hierarchy={use_preferrence_hierarchy}, use_preferred_operators_conjunctive={use_preferred_operators_conjunctive})",
                 "--evaluator", "hff=ff(transform=adapt_costs(one))",
                 "--search",
                 "lazy_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,reopen_closed=false)"]),
            IssueConfig(
                f"lm_hm2-hierarchy={use_preferrence_hierarchy}-conjunctive={use_preferred_operators_conjunctive}",
                ["--evaluator",
                 f"hlm=lmcount(lm_factory=lm_hm(m=2),pref=true, use_preferrence_hierarchy={use_preferrence_hierarchy}, use_preferred_operators_conjunctive={use_preferred_operators_conjunctive})",
                 "--search",
                 "eager_greedy([hlm],preferred=[hlm])"]),
        ]


SUITE = common_setup.DEFAULT_SATISFICING_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="simon.dold@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=3)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("landmark_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("landmarks_disjunctive", min_wins=False),
    Attribute("landmarks_conjunctive", min_wins=False),
    Attribute("orderings", min_wins=False),
    Attribute("lmgraph_generation_time", min_wins=True),
]

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch', merge=True)

exp.add_absolute_report_step(attributes=ATTRIBUTES)
exp.add_scatter_plot_step(relative=True, attributes=["search_time", "cost"])
exp.add_scatter_plot_step(relative=False, attributes=["search_time", "cost"])

exp.add_archive_step(ARCHIVE_PATH)

exp.run_steps()
