#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11
from downward.configs import default_configs_optimal
from downward.reports.scatter import ScatterPlotReport

import common_setup


REVS = ["issue214-base", "issue214-v3"]
CONFIGS = {"ipdb": ["--search", "astar(ipdb())"]}

TEST_RUN = True

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = suite_optimal_with_ipc11()
    PRIORITY = 0     # number means maia experiment


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

exp()
