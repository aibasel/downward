#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup
import configs

CONFIGS = {}
INCLUDE = ("lazy", "lama")
EXCLUDE = ("lazy_greedy_add", "lazy_greedy_cea", "lazy_greedy_cg")
for key, value in configs.default_configs_satisficing(ipc=False, extended=True).items():
    if any(x in key for x in INCLUDE) and not any(x in key for x in EXCLUDE):
        CONFIGS[key] = value
print(sorted(CONFIGS.keys()))
print(len(CONFIGS))

SUITE = downward.suites.suite_satisficing_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v5-base", "issue77-v5"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
