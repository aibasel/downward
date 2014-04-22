#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


exp = common_setup.IssueExperiment(
    search_revisions=["issue422-base", "issue422-v1"],
    configs={"lmcut": ["--search", "astar(lmcut())"]},
    suite=suites.suite_optimal_with_ipc11(),
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
