#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""
Before you can run the experiment you need to create duplicates of the
two tasks we want to test:

cd ../benchmarks/tidybot-opt11-strips
for i in {00..49}; do cp p14.pddl p14-$i.pddl; done

cd ../parking-opt11-strips
for i in {00..49}; do cp pfile04-015.pddl pfile04-015-$i.pddl; done

Don't forget to remove the duplicate tasks afterwards. Otherwise they
will be included in subsequent experiments.
"""

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
