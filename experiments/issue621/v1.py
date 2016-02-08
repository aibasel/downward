#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


configs = [
    IssueConfig(
        "cegar-10K-original",
        ["--search", "astar(cegar(subtasks=[original()],max_states=10000,max_time=infinity))"]),
]

exp = IssueExperiment(
    revisions=["issue621-base", "issue621-v1"],
    configs=configs,
    suite=suites.suite_optimal_with_ipc11(),
    test_suite=["depot:pfile1"],
    email="jendrik.seipp@unibas.ch",
)

exp.add_comparison_table_step()

exp()
