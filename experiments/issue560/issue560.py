#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from relativescatter import RelativeScatterPlotReport
import common_setup


REVS = ["issue560-base", "issue560-v1"]
SUITE = suites.suite_all()

# We are only interested in the preprocessing here and will only run the first steps of the experiment.
CONFIGS = {
    "astar_blind": [
        "--search",
        "astar(blind())"],
}

exp = common_setup.IssueExperiment(
    search_revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    )

exp.add_report(
    RelativeScatterPlotReport(
        attributes=["preprocess_wall_clock_time"],
        get_category=lambda run1, run2: run1.get("domain"),
    ),
    outfile='issue560_base_v1_preprocess_wall_clock_time.png'
)

exp.add_absolute_report_step(attributes=["preprocess_wall_clock_time"])

exp()
