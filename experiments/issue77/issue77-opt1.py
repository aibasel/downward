#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.configs
import downward.suites

# "ipc=False" skips portfolio configurations which we don't need to
# test here.
CONFIGS = downward.configs.default_configs_optimal(ipc=False, extended=True)

# pathmax is gone in this branch, remove it:
for key, value in list(CONFIGS.items()):
    for pos, arg in enumerate(value):
        if ", pathmax=false" in arg:
            value[pos] = arg.replace(", pathmax=false", "")

# selmax is currently disabled
del CONFIGS["astar_selmax_lmcut_lmcount"]

SUITE = downward.suites.suite_optimal_with_ipc11()

import common_setup

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-base", "issue77-v2"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
