#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup
import configs

NICKS = [
    'eager_greedy_alt_ff_cg', 'eager_greedy_ff', 'eager_greedy_ff_no_pref',
    'eager_pareto_ff', 'eager_wa3_cg'
]
CONFIGS = {}
for nick in NICKS:
    CONFIGS[nick] = configs.default_configs_satisficing(ipc=False, extended=True)[nick]

print(sorted(CONFIGS.keys()))
print(len(CONFIGS))

SUITE = downward.suites.suite_satisficing_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v7-base", "issue77-v7"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
