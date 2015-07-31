#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue558-base", "issue558-v1"]
LIMITS = {"search_time": 300}
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = {
    "lazy_wa3_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "lazy_wastar(h,w=3,preferred=h)"],
    "lama-w5": [
        "--heuristic",
        "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,"
        "                         lm_cost_type=plusone,cost_type=plusone))",
        "--search", "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5)"]
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
