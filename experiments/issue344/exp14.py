#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.configs
import downward.suites

import common_setup


exp = common_setup.MyExperiment(
    grid_priority=0,
    search_revisions=["issue344-base", "issue344-v5"],
    configs=downward.configs.default_configs_optimal(),
    suite=downward.suites.suite_optimal_with_ipc11(),
    do_test_run="auto"
    )

exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
