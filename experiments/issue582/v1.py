#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from relativescatter import RelativeScatterPlotReport
import common_setup


REVS = ["issue582-base", "issue582-v1"]
SUITE = suites.suite_optimal_with_ipc11()

CONFIGS = {
    "astar_lmcut": [
        "--search",
        "astar(lmcut())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )

exp.add_report(
    RelativeScatterPlotReport(
        attributes=["total_time"],
        get_category=lambda run1, run2: run1.get("domain"),
    ),
    outfile='issue582_base_v1_total_time.png'
)

exp.add_comparison_table_step()

exp()
