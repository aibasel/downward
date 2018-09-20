#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup
import configs

CONFIGS = configs.default_configs_optimal(ipc=False, extended=False)

print(sorted(CONFIGS.keys()))
print(len(CONFIGS))


SUITE = downward.suites.suite_optimal_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v3", "issue77-v4"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
