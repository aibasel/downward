#! /usr/bin/env python
# -*- coding: utf-8 -*-


import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue383-base", "issue383-v1"]

CONFIGS = [
    IssueConfig(f"lama-no-syn-pref-{pref}", [
        "--if-unit-cost", "--evaluator",
        f"hlm=lmcount(lm_rhw(reasonable_orders=true), pref={pref})",
        "--evaluator", "hff=ff()",
        "--search", """iterated([
                        lazy_greedy([hff,hlm], preferred=[hff,hlm]),
                        lazy_wastar([hff,hlm], preferred=[hff,hlm], w=5),
                        lazy_wastar([hff,hlm], preferred=[hff,hlm], w=3),
                        lazy_wastar([hff,hlm], preferred=[hff,hlm], w=2),
                        lazy_wastar([hff,hlm], preferred=[hff,hlm], w=1)
                        ], repeat_last=true, continue_on_fail=true)""",
        "--if-non-unit-cost",
        "--evaluator",
        f"hlm1=lmcount(lm_rhw(reasonable_orders=true), transform=adapt_cost(one), preferred_operators={pref})",
        "--evaluator", "hff1=ff(transform=adapt_costs(one))",
        "--evaluator",
        f"hlm2=lmcount(lm_rhw(reasonable_orders=true), transform=adapt_costs(plusone), preferred_operators={pref})",
        "--evaluator", "hff2=ff(transform=adapt_costs(plusone))",
        "--search", """iterated([
                        lazy_greedy([hff1,hlm1], preferred=[hff1,hlm1],
                                    cost_type=one, reopen_closed=false),
                        lazy_greedy([hff2,hlm2], preferred=[hff2,hlm2],
                                    reopen_closed=false),
                        lazy_wastar([hff2,hlm2], preferred=[hff2,hlm2], w=5),
                        lazy_wastar([hff2,hlm2], preferred=[hff2,hlm2], w=3),
                        layz_wastar([hff2,hlm2], preferred=[hff2,hlm2], w=2),
                        layz_wastar([hff2,hlm2], preferred=[hff2,hlm2], w=1)
                        ], repeat_last=true, continue_on_fail=true)""",
        "--always"])
    for pref in ["true", "false"]
]

if common_setup.is_test_run() or not common_setup.is_running_on_cluster():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=2)
else:
    SUITE = common_setup.DEFAULT_SATISFICING_SUITE
    ENVIRONMENT = BaselSlurmEnvironment(
        partition="infai_2",
        email="clemens.buechner@unibas.ch",
        export=["PATH", "DOWNWARD_BENCHMARKS"],
    )

exp = common_setup.IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(exp.ANYTIME_SEARCH_PARSER)

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")
exp.add_comparison_table_step()

exp.run_steps()
