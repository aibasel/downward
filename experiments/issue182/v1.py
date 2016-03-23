#! /usr/bin/env python
# -*- coding: utf-8 -*-

from common_setup import IssueConfig, IssueExperiment
import suites


configs = [
    IssueConfig(
        func,
        ["--search", "astar({}([ipdb(max_time=5),diverse_potentials(),all_states_potential(),initial_state_potential(),sample_based_potentials()]))".format(func)])
    for func in ["max", "ipc_max"]
]
revisions = ["8f1563b36fc7"]

exp = IssueExperiment(
    revisions=revisions,
    configs=configs,
    suite=suites.suite_optimal_strips(),
    test_suite=["depot:pfile1"],
    email="jendrik.seipp@unibas.ch",
)

exp.add_absolute_report_step()

exp()
