#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup

CONFIGS = {
    "cg-lazy-nopref": [
        "--heuristic", "h=cg()",
        "--search", "lazy_greedy(h)"
        ],
    "cg-lazy-pref": [
        "--heuristic", "h=cg()",
        "--search", "lazy_greedy(h, preferred=[h])"
        ],
    }

exp = common_setup.IssueExperiment(
    search_revisions=["issue470-base", "issue470-v1"],
    configs=CONFIGS,
    suite=suites.suite_satisficing_with_ipc11(),
    )

exp.add_comparison_table_step()

exp()
