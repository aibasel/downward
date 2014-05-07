#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue392-v2"]
LIMITS = {"search_time": 300}

CONFIGS = {}
for randomize in ["false", "true"]:
    for pref_first in ["false", "true"]:
        CONFIGS["lama-nonunit-randomize-%(randomize)s-pref_first-%(pref_first)s" % locals()] = [
            "--heuristic", "hlm1,hff1=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=ONE,cost_type=ONE))",
            "--heuristic", "hlm2,hff2=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=PLUSONE,cost_type=PLUSONE))",
            "--search",
            "iterated(["
                "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],randomize_successors=%(randomize)s,preferred_successors_first=%(pref_first)s,cost_type=ONE,reopen_closed=false),"
                "lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],randomize_successors=%(randomize)s,preferred_successors_first=%(pref_first)s,reopen_closed=false),"
                "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],randomize_successors=%(randomize)s,preferred_successors_first=%(pref_first)s,w=5),"
                "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],randomize_successors=%(randomize)s,preferred_successors_first=%(pref_first)s,w=3),"
                "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],randomize_successors=%(randomize)s,preferred_successors_first=%(pref_first)s,w=2),"
                "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],randomize_successors=%(randomize)s,preferred_successors_first=%(pref_first)s,w=1)],"
                "repeat_last=true,continue_on_fail=true)" % locals()
        ]

SUITE = sorted(set(suites.suite_satisficing_with_ipc11()) &
               set(suites.suite_diverse_costs()))


exp = common_setup.IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )

exp.add_absolute_report_step()

exp()
