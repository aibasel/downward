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
REVISIONS = ["issue960-base", "issue960-v1"]
CONFIGS = [
    IssueConfig("opcount-seq-lmcut-soplex", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver=soplex))"]),
    IssueConfig("diverse-potentials-soplex", ["--search", "astar(diverse_potentials(lpsolver=soplex))"]),
    IssueConfig("optimal-lmcount-soplex", ["--search", "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]), admissible=true, optimal=true, lpsolver=soplex))"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="rik.degraaff@unibas.ch",
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
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

#exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step(relative=True, attributes=["total_time", "memory", "coverage"])

exp.run_steps()
