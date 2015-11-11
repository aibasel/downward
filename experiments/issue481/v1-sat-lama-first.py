#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


# Use the latest merged revision from "default" branch. The first
# revision on issue481 branch doesn't use CMake.
REVS = ["issue481-base", "issue481"]
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = {
    "ff": ["--search", "eager_greedy(ff())"],
}

exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
)
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
