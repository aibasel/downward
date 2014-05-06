#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue392-v1"]
LIMITS = {"search_time": 300}

CONFIGS = {}
for option in ["pref_first", "original", "shuffled", "shuffled_pref_first"]:
    CONFIGS["lama-unit-%s" % option] = [
        "--heuristic",
        "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=PLUSONE,cost_type=PLUSONE))",
        "--search",
        "iterated(["
            "lazy_greedy([hff,hlm],preferred=[hff,hlm],succ_order=%(option)s),"
            "lazy_wastar([hff,hlm],preferred=[hff,hlm],succ_order=%(option)s,w=5),"
            "lazy_wastar([hff,hlm],preferred=[hff,hlm],succ_order=%(option)s,w=3),"
            "lazy_wastar([hff,hlm],preferred=[hff,hlm],succ_order=%(option)s,w=2),"
            "lazy_wastar([hff,hlm],preferred=[hff,hlm],succ_order=%(option)s,w=1)],"
            "repeat_last=true,continue_on_fail=true)" % locals()
    ]

TEST_RUN = False

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = sorted(set(suites.suite_satisficing_with_ipc11()) &
                   set(suites.suite_unit_costs()))
    PRIORITY = 0     # number means maia experiment


exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_absolute_report_step()

exp()
