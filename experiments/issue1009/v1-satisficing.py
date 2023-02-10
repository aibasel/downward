#! /usr/bin/env python
# -*- coding: utf-8 -*-

import common_setup
import itertools

from common_setup import IssueConfig, IssueExperiment

import os

from lab.reports import Attribute

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

CONFIGS_COMMON = [
    common_setup.IssueConfig(
        "lama-first", [], driver_options=["--alias", "lama-first"]),
    common_setup.IssueConfig(
        "lama-first-pref", ["--evaluator",
                            "hlm=lmcount(lm_factory=lm_reasonable_orders_hps("
                            "lm_rhw()),transform=adapt_costs(one),pref=true)",
                            "--evaluator", "hff=ff(transform=adapt_costs(one))",
                            "--search",
                            """lazy_greedy([hff,hlm],preferred=[hff,hlm], cost_type=one,reopen_closed=false)"""]),
    common_setup.IssueConfig(
        "lama-second", ["--evaluator",
                            "hlm=lmcount(lm_factory=lm_reasonable_orders_hps("
                            "lm_rhw()),transform=adapt_costs(plusone))",
                            "--evaluator", "hff=ff(transform=adapt_costs(plusone))",
                            "--search",
                            """lazy_greedy([hff,hlm],preferred=[hff,hlm], reopen_closed=false)"""]),
]
CONFIGS_OLD = [
    common_setup.IssueConfig(
        "lm-merged", ["--search",
                      "eager_greedy([lmcount(lm_merged([lm_exhaust(), lm_zg()]))])"]),
    common_setup.IssueConfig(
        "lm-exhaust", ["--search",
                       "eager_greedy([lmcount(lm_exhaust())])"]),
    common_setup.IssueConfig(
        "lm-zg", ["--search",
                  "eager_greedy([lmcount(lm_zg())])"]),
    common_setup.IssueConfig(
        "lm-hm", ["--search",
                  "eager_greedy([lmcount(lm_hm(m=2))])"]),
]
CONFIGS_NEW = [
    common_setup.IssueConfig(
        "lm-merged", ["--search",
                      "eager_greedy([lmcount(lm_merged([lm_exhaust(), lm_zg()]), transform=adapt_costs(one))])"]),
    common_setup.IssueConfig(
        "lm-exhaust", ["--search",
                       "eager_greedy([lmcount(lm_exhaust(), transform=adapt_costs(one))])"]),
    common_setup.IssueConfig(
        "lm-zg", ["--search",
                  "eager_greedy([lmcount(lm_zg(), transform=adapt_costs(one))])"]),
    common_setup.IssueConfig(
        "lm-hm", ["--search",
                  "eager_greedy([lmcount(lm_hm(m=2), transform=adapt_costs(one))])"]),
]

revconf_base = ("issue1009-base", CONFIGS_COMMON + CONFIGS_OLD)
revconf_v1 = ("issue1009-v1", CONFIGS_COMMON + CONFIGS_NEW)
revconf_v2 = ("issue1009-v2", CONFIGS_COMMON + CONFIGS_NEW)
REVCONFS = [revconf_base, revconf_v1, revconf_v2]


BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REPO = os.environ["DOWNWARD_REPO"]

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

exp = common_setup.IssueExperiment(revisions=[], configs=[], environment=ENVIRONMENT)

# Manually add relevant revision/config combinations to algorithms
for revconv in REVCONFS:
    for config in revconv[1]:
        exp.add_algorithm(
            common_setup.get_algo_nick(revconv[0], config.nick),
            common_setup.get_repo_base(),
            revconv[0],
            config.component_options,
            build_options=config.build_options,
            driver_options=config.driver_options)

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser("landmark_parser.py")

ATTRIBUTES = IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("landmarks", min_wins=False),
    Attribute("disjunctive_landmarks", min_wins=False),
    Attribute("conjunctive_landmarks", min_wins=False),
    Attribute("orderings", min_wins=False),
]

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

def make_comparison_tables():
    for revconf1, revconf2 in itertools.combinations(REVCONFS, 2):
        compared_configs = []
        for config1, config2 in zip(revconf1[1], revconf2[1]):
            compared_configs.append(
                ("{0}-{1}".format(revconf1[0], config1.nick),
                 "{0}-{1}".format(revconf2[0], config2.nick)))
        report = common_setup.ComparativeReport(compared_configs, attributes=ATTRIBUTES)
        outfile = os.path.join(
            exp.eval_dir,
            "{0}-{1}-{2}-compare.{3}".format(exp.name, revconf1[0], revconf2[0], report.output_format))
        report(exp.eval_dir, outfile)

exp.add_step("make-comparison-tables", make_comparison_tables)
exp.add_parse_again_step()

exp.run_steps()
