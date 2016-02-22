#! /usr/bin/env python
# -*- coding: utf-8 -*-

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport
import suites


configs = [
    IssueConfig(
        "cegar-900s",
        ["--search", "astar(cegar(subtasks=[landmarks(),goals()],max_time=900))"]),
]
revisions = ["issue632-base", "issue632-v1"]

exp = IssueExperiment(
    revisions=revisions,
    configs=configs,
    suite=suites.suite_optimal_with_ipc11(),
    test_suite=["depot:pfile1"],
    email="jendrik.seipp@unibas.ch",
)

exp.add_comparison_table_step()

for attribute in ["memory", "total_time"]:
    for config in configs:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=[attribute],
                filter_config=["{}-{}".format(rev, config.nick) for rev in revisions],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile="{}-{}-{}.png".format(exp.name, attribute, config.nick)
        )

exp()
