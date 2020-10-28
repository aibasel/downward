#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from downward.reports.compare import ComparativeReport
from downward.reports.scatter import ScatterPlotReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue891-v1"]
CONFIGS = [
    IssueConfig("opcount-lp",  ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver=cplex, use_integer_operator_counts=false))"]),
    IssueConfig("opcount-mip", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver=cplex, use_integer_operator_counts=true))"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
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

exp.add_report(ComparativeReport(
    [("issue891-v1-opcount-lp", "issue891-v1-opcount-mip", "Diff (LP/MIP)")],
    attributes=exp.DEFAULT_TABLE_ATTRIBUTES + ["initial_h_value"]))

exp.add_scatter_plot_step(relative=False, attributes=["total_time", "memory"],
    additional=[
        ("opcount-lp", "opcount-mip", "issue891-v1", "issue891-v1", "total_time"),
        ("opcount-lp", "opcount-mip", "issue891-v1", "issue891-v1", "memory"),
    ])

def interesting_h_value(run):
    if "initial_h_value" in run and run["initial_h_value"] > 50:
        run["initial_h_value"] = 51
    return run

exp.add_report(ScatterPlotReport(
    attributes=["initial_h_value"],
    filter=interesting_h_value,
    get_category=lambda run1, run2: run1["domain"],
))

exp.run_steps()
