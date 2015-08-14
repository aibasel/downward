#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
import common_setup


REVS = ["issue67-v1-base", "issue67-v1"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "astar_blind": [
        "--search",
        "astar(blind())"],
    "astar_lmcut": [
        "--search",
        "astar(lmcut())"],
    "astar_lm_zg": [
        "--search",
        "astar(lmcount(lm_zg(), admissible=true, optimal=true))"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )
exp.add_comparison_table_step()

exp()
