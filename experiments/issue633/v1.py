#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


configs = [
    IssueConfig(
        "cegar-10K-original",
        ["--search", "astar(cegar(subtasks=[original()],max_states=10000,max_time=infinity))"]),
    IssueConfig(
        "cegar-10K-landmarks-goals",
        ["--search", "astar(cegar(subtasks=[landmarks(), goals()],max_states=10000,max_time=infinity))"]),
    IssueConfig(
        "cegar-900s-landmarks-goals",
        ["--search", "astar(cegar(subtasks=[landmarks(), goals()],max_states=infinity,max_time=900))"]),
]

exp = IssueExperiment(
    revisions=["issue633-base", "issue633-v1"],
    configs=configs,
    suite=suites.suite_optimal_with_ipc11(),
    test_suite=["depot:pfile1"],
    email="jendrik.seipp@unibas.ch",
)

exp.add_comparison_table_step()

exp()
