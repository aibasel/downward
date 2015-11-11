#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


# Use the latest merged revision from "default" branch. The first
# revision on issue481 branch doesn't use CMake.
REVS = ["issue481-base", "issue481"]
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = [
    IssueConfig("ff", ["--search", "eager_greedy(ff())"]),
    # This config shows how to pass driver options.
    # It fails since lazy_wastar is currently missing.
    IssueConfig("lama", [], driver_options=["--alias", "seq-sat-lama-2011"]),
]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
)
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
