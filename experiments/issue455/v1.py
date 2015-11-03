#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue455"]
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = {
    "01-ff": [
        "--heuristic",
            "hff=ff(cost_type=one)",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true)]),"
            "preferred=[hff],cost_type=one)"
        ],
    "02-ff-type-const": [
        "--heuristic",
            "hff=ff(cost_type=one)",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), type_based([const(1)])]),"
            "preferred=[hff],cost_type=one)"
        ],
    "03-lama-first": [
        "--heuristic",
            "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=one,cost_type=one))",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)]),"
            "preferred=[hff,hlm],cost_type=one)"
        ],
    "04-lama-first-types-ff-g": [
        "--heuristic",
            "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,lm_cost_type=one,cost_type=one))",
        "--search",
            "lazy(alt([single(hff),single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([hff, g()])]),"
            "preferred=[hff,hlm],cost_type=one)"
        ],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )
exp.add_absolute_report_step()

exp()
