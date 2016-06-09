#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue592-base", "issue592-v2"]
SUITE = suites.suite_optimal_strips()

CONFIGS = [
    IssueConfig("seq-opt-bjolp", [], driver_options=["--alias", "seq-opt-bjolp"]),
]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="manuel.heusner@unibas.ch"
)

exp.add_comparison_table_step()

exp()
