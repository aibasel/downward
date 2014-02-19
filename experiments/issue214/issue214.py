#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal

import common_setup


REVS = ["issue214-base", "issue214-v2"]
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


# TODO: I'd like to specify "search_revisions" (which uses the same
# translator and preprocessor for everything) instead of "revisions"
# here, but I can't seem to make this work with the REVS argument for
# CompareRevisionsReport.

exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    parsers=['state_size_parser.py'],
    )


exp.add_comparison_table_step(
    attributes=common_setup.MyExperiment.DEFAULT_TABLE_ATTRIBUTES + ['bytes_per_state', 'variables', 'state_var_t_size']
)
exp.add_scatter_plot_step()

exp()
