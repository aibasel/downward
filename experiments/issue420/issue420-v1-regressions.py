#! /usr/bin/env python
# -*- coding: utf-8 -*-

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
    SUITE = (["tidybot-opt11-strips:p14-%02d.pddl" % i for i in range(50)] +
             ["parking-opt11-strips:pfile04-015-%02d.pddl" % i for i in range(50)])
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
