#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["issue752-v1"]
CONFIGS = [
    IssueConfig('astar-seq-cplex', ["--search", "astar(operatorcounting([state_equation_constraints()], lpsolver=cplex))"],
        build_options=["release64"], driver_options=["--build", "release64"]),
    IssueConfig('astar-seq-soplex', ["--search", "astar(operatorcounting([state_equation_constraints()], lpsolver=soplex))"],
        build_options=["release64"], driver_options=["--build", "release64"]),
    IssueConfig('astar-seq-pho-cplex', ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver=cplex))"],
        build_options=["release64"], driver_options=["--build", "release64"]),
    IssueConfig('astar-seq-pho-soplex', ["--search", "astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver=soplex))"],
        build_options=["release64"], driver_options=["--build", "release64"]),
    IssueConfig('astar-seq-lmcut-cplex', ["--search", "astar(operatorcounting([state_equation_constraints(), pho_constraints(patterns=systematic(2))], lpsolver=cplex))"],
        build_options=["release64"], driver_options=["--build", "release64"]),
    IssueConfig('astar-seq-lmcut-soplex', ["--search", "astar(operatorcounting([state_equation_constraints(), pho_constraints(patterns=systematic(2))], lpsolver=soplex))"],
        build_options=["release64"], driver_options=["--build", "release64"]),
]
SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(email="florian.pommerening@unibas.ch", export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_absolute_report_step()

for attribute in ["total_time"]:
    for config in ["astar-seq-pho", "astar-seq-lmcut"]:
        for rev in REVISIONS:
            exp.add_report(
                RelativeScatterPlotReport(
                    attributes=[attribute],
                    filter_algorithm=["{}-{}-{}".format(rev, config, solver) for solver in ["cplex", "soplex"]],
                    get_category=lambda run1, run2: run1.get("domain"),
                ),
                outfile="{}-{}-{}.png".format(exp.name, attribute, config)
            )

exp.run_steps()
