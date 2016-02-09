#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport


configs = [
    IssueConfig(
        "cegar-10K-original",
        ["--search", "astar(cegar(subtasks=[original()],max_states=10000,max_time=infinity))"]),
]

exp = IssueExperiment(
    revisions=["issue621-base", "issue621-v1"],
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
                filter_config=[
                    "issue627-base-%s" % config.nick,
                    "issue627-v1-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile="issue621_base_v1_{}_{}.png".format(attribute, config.nick)
        )

exp()
