#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport
from itertools import combinations

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
# These revisions are all tag experimental branches off the same revision.
# we only need different tags so lab creates separate build directories in the build cache.
# We then manually recompile the code in the build cache with the correct settings.
REVISIONS = ["issue925-cplex12.8-static", "issue925-cplex12.8-dynamic", "issue925-cplex12.9-static", "issue925-cplex12.9-dynamic"]
CONFIGS = [
    IssueConfig("opcount-seq-lmcut", ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()]))"]),
    IssueConfig("diverse-potentials", ["--search", "astar(diverse_potentials())"]),
    IssueConfig("optimal-lmcount", ["--search", "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]), admissible=true, optimal=true))"]),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(email="florian.pommerening@unibas.ch")

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

exp.add_comparison_table_step()

for r1, r2 in combinations(REVISIONS, 2):
    for nick in ["opcount-seq-lmcut", "diverse-potentials", "optimal-lmcount"]:
        exp.add_report(RelativeScatterPlotReport(
            attributes=["total_time"],
            filter_algorithm=["%s-%s" % (r, nick) for r in [r1, r2]],
            get_category=lambda run1, run2: run1["domain"]),
            outfile="issue925-v1-total-time-%s-%s-%s.png" % (r1, r2, nick))

exp.run_steps()
