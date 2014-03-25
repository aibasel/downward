#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal

import common_setup


REVS = ["issue422-base", "issue422-v1"]
CONFIGS = default_configs_optimal()
SUITE = suite_optimal_with_ipc11()
PRIORITY = 0

# Remove config that is disabled in this branch.
del CONFIGS["astar_selmax_lmcut_lmcount"]


exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )


exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
