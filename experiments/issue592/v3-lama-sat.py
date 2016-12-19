#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue592-base", "issue592-v3"]
SUITE = suites.suite_satisficing()

CONFIGS = [
    IssueConfig("seq-sat-lama-2011", [], driver_options=["--alias", "seq-sat-lama-2011"]),
    IssueConfig("lama-first", [], driver_options=["--alias", "lama-first"]),
]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="manuel.heusner@unibas.ch"
)

exp.add_comparison_table_step()

exp()
