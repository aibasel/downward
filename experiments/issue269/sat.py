#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

import common_setup


REVS = ["issue269-base", "issue269-v1"]
LIMITS = {"search_time": 600}
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = {
    "random-successors": ["--search", "lazy_greedy(ff(),randomize_successors=true)"],
    "pareto-open-list": [
        "--heuristic", "h=ff()",
        "--search", "eager(pareto([sum([g(), h]), h]), reopen_closed=true, pathmax=false,f_eval=sum([g(), h]))"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    limits=LIMITS,
    )
exp.add_comparison_table_step()

exp()
