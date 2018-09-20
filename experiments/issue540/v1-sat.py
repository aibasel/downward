#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue540-base", "issue540-v1"]
LIMITS = {"search_time": 300}
SUITE = suites.suite_satisficing_with_ipc11()
CONFIGS = {
    "blind": ["--search", "astar(blind())"],
    "lama-first": [
        "--if-unit-cost",
        "--heuristic",
            "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true))",
        "--search",
            "lazy_greedy([hff,hlm],preferred=[hff,hlm])",
        "--if-non-unit-cost",
        "--heuristic",
            "hlm1,hff1=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=one,cost_type=one))",
        "--search",
            "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],cost_type=one,reopen_closed=false)",
        "--always"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
