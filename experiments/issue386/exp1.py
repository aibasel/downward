#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal

import common_setup


REVS = ["issue386-base", "issue386-v1"]
CONFIGS = {'astar_ipdb': ['--search', 'astar(ipdb)']} # default_configs_optimal()

TEST_RUN = True

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = suite_optimal_with_ipc11()
    PRIORITY = 0     # number means maia experiment


# TODO: I'd like to specify "search_revisions" (which uses the same
# translator and preprocessor for everything) instead of "revisions"
# here, but I can't seem to make this work with the REVS argument for
# CompareRevisionsReport.

exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE
    )


exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
