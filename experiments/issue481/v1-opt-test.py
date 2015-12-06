#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue481-base", "issue481-v1"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = [
    # Greedy (tests single and alternating open lists)
    IssueConfig("astar_lmcut", [
        "--search",
            "astar(lmcut())"
        ]),
]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="malte.helmert@unibas.ch"
)

exp.add_comparison_table_step()

exp()
