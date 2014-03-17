#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal

import common_setup


REVS = ["issue422-base", "issue422-v1"]
CONFIGS = default_configs_optimal()

# remove config that is disabled in this branch
del CONFIGS['astar_selmax_lmcut_lmcount']

TEST_RUN = True

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = suite_optimal_with_ipc11()
    PRIORITY = 0     # number means maia experiment


exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )


exp.add_comparison_table_step(
    attributes=common_setup.MyExperiment.DEFAULT_TABLE_ATTRIBUTES
)
exp.add_scatter_plot_step()

exp()
