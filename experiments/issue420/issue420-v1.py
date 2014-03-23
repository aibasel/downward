#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_optimal_with_ipc11

import common_setup


REVS = ["issue420-base", "issue420-v1"]
CONFIGS = {
    "blind": ["--search", "astar(blind())"],
    "lmcut": ["--search", "astar(lmcut())"],
}

TEST_RUN = False

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
    )

exp.add_comparison_table_step(
    attributes=common_setup.MyExperiment.DEFAULT_TABLE_ATTRIBUTES
)

exp()
