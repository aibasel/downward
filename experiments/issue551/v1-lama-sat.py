#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
from downward import suites
from common_setup import IssueConfig, IssueExperiment


REVS = ["issue551-base", "issue551-v1"]
BENCHMARKS = os.path.expanduser('~/downward-benchmarks')
SUITE = suites.suite_satisficing()

CONFIGS = [
    IssueConfig("seq-sat-lama-2011", [], driver_options=["--alias", "seq-sat-lama-2011"]),
    IssueConfig("lama-first", [], driver_options=["--alias", "lama-first"]),
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
