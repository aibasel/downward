#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue983-v1"]
CONFIGS = [
    IssueConfig("opcount-hplus", ["--search",
        "astar(operatorcounting(["
            "delete_relaxation_constraints(use_time_vars=true, use_integer_vars=true)"
        "], use_integer_operator_counts=True), bound=0)"]),
    IssueConfig("opcount-hplus-lmcut", ["--search",
        "astar(operatorcounting(["
            "delete_relaxation_constraints(use_time_vars=true, use_integer_vars=true),"
            "lmcut_constraints()"
        "], use_integer_operator_counts=True), bound=0)"]),
    IssueConfig("relaxed-lmcut", ["--search", "astar(lmcut())", "--translate-options",  "--relaxed"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_1",
    email="florian.pommerening@unibas.ch",
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

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')


def add_hplus(run):
    if "hplus" in run["algorithm"]:
        run["hplus"] = run.get("initial_h_value")
    else:
        run["hplus"] = run.get("cost")
    return run

exp.add_report(ComparativeReport([
        ("issue983-v1-opcount-hplus", "issue983-v1-opcount-hplus-lmcut", "Diff (adding landmarks)"),
        ("issue983-v1-opcount-hplus", "issue983-v1-relaxed-lmcut", "Diff (MIP/search)"),
        ("issue983-v1-opcount-hplus-lmcut", "issue983-v1-relaxed-lmcut", "Diff (MIP+LM/search)"),
    ],
    filter=add_hplus,
    attributes=exp.DEFAULT_TABLE_ATTRIBUTES + ["initial_h_value", "hplus"]))

exp.add_scatter_plot_step(relative=False, attributes=["total_time", "memory"],
    additional=[
        ("opcount-hplus", "opcount-hplus-lmcut", "issue983-v1", "issue983-v1", "total_time"),
        ("opcount-hplus", "opcount-hplus-lmcut", "issue983-v1", "issue983-v1", "memory"),
        ("opcount-hplus", "relaxed-lmcut", "issue983-v1", "issue983-v1", "total_time"),
        ("opcount-hplus", "relaxed-lmcut", "issue983-v1", "issue983-v1", "memory"),
        ("opcount-hplus-lmcut", "relaxed-lmcut", "issue983-v1", "issue983-v1", "total_time"),
        ("opcount-hplus-lmcut", "relaxed-lmcut", "issue983-v1", "issue983-v1", "memory"),
    ])

exp.run_steps()
