#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal

import common_setup


REVS = ["issue344-base", "issue344-v5"]
CONFIGS = default_configs_optimal()

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
    suite=SUITE
    )


exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
