#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
from downward import suites
from common_setup import IssueConfig, IssueExperiment


REVS = ["issue551-base", "issue551-v3"]
BENCHMARKS = os.path.expanduser('~/downward-benchmarks')
SUITE = suites.suite_optimal_strips()

CONFIGS = [
    IssueConfig("seq-opt-bjolp", [], driver_options=["--alias", "seq-opt-bjolp"]),
]

exp = IssueExperiment(
    revisions=REVS,
    benchmarks_dir=BENCHMARKS,
    suite=SUITE,
    configs=CONFIGS,
    processes=4,
    email="manuel.heusner@unibas.ch"
)

exp.add_comparison_table_step()

exp()
