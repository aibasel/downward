#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal

import common_setup


configs = default_configs_optimal()
# Remove config that is disabled in this branch.
del configs["astar_selmax_lmcut_lmcount"]


exp = common_setup.IssueExperiment(
    grid_priority=0,
    #search_revisions=["issue422-base", "issue422-v1"],
    configs=configs,
    suite=suite_optimal_with_ipc11(),
    )


exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
