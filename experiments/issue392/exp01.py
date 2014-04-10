#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_satisficing_with_ipc11
from downward.reports.absolute import AbsoluteReport

import common_setup


REVS = ["issue392"]
LIMITS = {"search_time": 300}

CONFIGS = {}
for option in ["pref_first", "original", "shuffled", "shuffled_pref_first"]:
    CONFIGS["lamafirst-%s" % option] = [
        "--heuristic",
        "hlm1,hff1=lm_ff_syn(lm_rhw(reasonable_orders=true," +
        "lm_cost_type=ONE,cost_type=ONE))",
        "--search",
        "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1]," +
        "cost_type=ONE,reopen_closed=false," +
        "succ_order=%s)" % option.upper()
    ]
    CONFIGS["lazy-greedy-ff-%s" % option] = [
        "--heuristic",
        "hff=ff()",
        "--search",
        "lazy_greedy(hff,preferred=[hff],reopen_closed=true,succ_order=%s)" %
            option.upper()
    ]
    CONFIGS["iterated-lazy-greedy-ff-%s" % option] = [
        "--heuristic",
        "hff=ff()",
        "--search",
        "iterated(lazy_greedy(hff,preferred=[hff],reopen_closed=true,succ_order=%s),repeat_last=true)" %
            option.upper()
    ]

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
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_report(AbsoluteReport(
    attributes=common_setup.MyExperiment.DEFAULT_TABLE_ATTRIBUTES))

exp()
