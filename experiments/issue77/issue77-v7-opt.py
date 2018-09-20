#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup
import configs

CONFIGS = configs.default_configs_optimal(ipc=False, extended=False)

print(sorted(CONFIGS.keys()))
print(len(CONFIGS))

SUITE = downward.suites.suite_optimal_with_ipc11()
SCATTER_ATTRIBUTES = ["total_time"]

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v7-base", "issue77-v7"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step(attributes=SCATTER_ATTRIBUTES, relative=True)

exp()
