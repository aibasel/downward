#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
import common_setup
from relativescatter import RelativeScatterPlotReport


REVS = ["issue67-v4-base", "issue67-v4"]
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

exp.add_report(
    RelativeScatterPlotReport(
        attributes=["total_time"],
        get_category=lambda run1, run2: run1.get("domain"),
    ),
    outfile='issue67-v4-total-time.png'
)

exp()
