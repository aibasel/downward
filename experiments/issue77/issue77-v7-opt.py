#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup
import configs
import relative_scatter

CONFIGS = configs.default_configs_optimal(ipc=False, extended=False)

print(sorted(CONFIGS.keys()))
print(len(CONFIGS))


SUITE = downward.suites.suite_optimal_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v7-base", "issue77-v7"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step(attributes=["total_time"])
exp.add_report(relative_scatter.RelativeScatterPlotReport(
        filter_config=["issue77-v7-base-astar_blind", "issue77-v7-astar_blind"],
        attributes=["total_time"],
        get_category=lambda run1, run2: run1["domain"],),
    outfile="astar_blind-total_time.png")

exp()
