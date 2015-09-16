#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue544-base", "issue544-v1"]
LIMITS = {"search_time": 1800}
SUITE = suites.suite_satisficing_with_ipc11()
CONFIGS = {
    "eager_greedy_add": [
        "--heuristic",
        "h=add()",
        "--search",
        "eager_greedy(h, preferred=h)"],
    "eager_greedy_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "eager_greedy(h, preferred=h)"],
    "lazy_greedy_add": [
        "--heuristic",
        "h=add()",
        "--search",
        "lazy_greedy(h, preferred=h)"],
    "lazy_greedy_ff": [
        "--heuristic",
        "h=ff()",
        "--search",
        "lazy_greedy(h, preferred=h)"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
