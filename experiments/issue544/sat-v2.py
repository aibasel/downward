#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue544-base-v2", "issue544-v2"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()
CONFIGS = {
    "eager_greedy_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "eager_greedy(h, preferred=h)"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
