#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_satisficing_with_ipc11

import common_setup


# HACK HACK HACK -- stupid of course -- we don't want to do a
# comparison experiment. But I don't really know how to set something
# else up quickly at the moment, and the way this is set up here, it
# should at least go fast because all the default runs will complain
# about usage errors.

REVS = ["default", "issue392"]

CONFIGS = {}
for option in ["pref_first", "original", "shuffled"]:
    name = "lamafirst-%s" % option
    args = [
        "--heuristic",
        "hlm1,hff1=lm_ff_syn(lm_rhw(reasonable_orders=true," +
        "lm_cost_type=ONE,cost_type=ONE))",
        "--search",
        "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"+
        "cost_type=ONE,reopen_closed=false," +
        "succ_mode=%s)" % option.upper()
        ]
    CONFIGS[name] = args

TEST_RUN = False

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = suite_satisficing_with_ipc11()
    PRIORITY = 0     # number means maia experiment


exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE
    )


exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
