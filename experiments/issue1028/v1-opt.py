#! /usr/bin/env python3

import os
from pathlib import Path

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = Path(__file__).resolve().parent
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue1028-base", "issue1028-v1"]
CONFIGS = [
    IssueConfig("lmcut", ["--search", "astar(lmcut())"]),
    IssueConfig("cartesian", ["--search", "astar(cegar())"]),
    IssueConfig("ipdb", ["--search", "astar(ipdb())"]),
    IssueConfig("merge-and-shrink", ["--search", "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50k,threshold_before_merge=1))"]),
    IssueConfig("opcount-seq-lmcut", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver=cplex))"]),
    IssueConfig("diverse-potentials", ["--search", "astar(diverse_potentials(lpsolver=cplex,random_seed=1729))"]),
    IssueConfig("optimal-lmcount", ["--search", "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]), admissible=true, optimal=true, lpsolver=cplex))"]),
]

SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="jendrik.seipp@liu.se",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=2)

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
exp.add_scatter_plot_step(relative=True, attributes=["total_time", "memory"])

exp.run_steps()
