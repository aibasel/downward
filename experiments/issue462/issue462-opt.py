#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites, configs

import common_setup

CONFIGS = configs.default_configs_optimal()

# remove config that is disabled in this branch
del CONFIGS['astar_selmax_lmcut_lmcount']

exp = common_setup.IssueExperiment(
    search_revisions=["issue462-base", "issue462-v1"],
    configs=CONFIGS,
    suite=suites.suite_optimal_with_ipc11(),
    limits={"search_time": 300}
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
