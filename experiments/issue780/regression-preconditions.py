#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue780-v1", "issue780-v2", "issue780-v3"]
MAX_DIV_TIME = 1 if common_setup.is_test_run() else 200
CONFIGS = [
    IssueConfig(
        "scp-sys2-50-orders",
        ["--search", "astar(saturated_cost_partitioning([projections(systematic(2))], diversify=false, max_orders=50, max_time=infinity, max_optimization_time=0))".format(**locals())]),
    IssueConfig(
        "scp-combined-div",
        ["--search", "astar(saturated_cost_partitioning([projections(hillclimbing(max_time=60, random_seed=0)), projections(systematic(2)), cartesian()], max_orders=infinity, max_time={MAX_DIV_TIME}, diversify=True, max_optimization_time=2, orders=greedy_orders(scoring_function=max_heuristic_per_stolen_costs), random_seed=0))".format(**locals())])
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)

#exp.add_absolute_report_step()
exp.add_comparison_table_step()

for attribute in ["memory"]:
    for config in CONFIGS:
        for rev1, rev2 in itertools.combinations(REVISIONS, 2):
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_algorithm=["{}-{}".format(rev, config.nick) for rev in (rev1, rev2)],
                    get_category=lambda run1, run2: run1.get("domain"),
                ),
                outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, rev1, rev2)
            )

exp.run_steps()
