#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites, configs
from downward.reports.compare import CompareConfigsReport

import common_setup

REVISIONS = ["issue462-base", "issue462-v1"]

exp = common_setup.IssueExperiment(
    search_revisions=REVISIONS,
    configs=configs.default_configs_satisficing(),
    suite=suites.suite_satisficing_with_ipc11(),
    limits={"search_time": 300}
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()

def grouped_configs_to_compare(config_nicks):
    grouped_configs = []
    for config_nick in config_nicks:
        col_names = ['%s-%s' % (r, config_nick) for r in REVISIONS]
        grouped_configs.append((col_names[0], col_names[1],
                           'Diff - %s' % config_nick))
    return grouped_configs

exp.add_report(CompareConfigsReport(
                    compared_configs=grouped_configs_to_compare(configs.configs_satisficing_core()),
                    attributes=common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES,
                ),
                outfile="issue462-sat-compare-core-configs.html"
)

def add_first_run_search_time(run):
    if  run.get("search_time_all", []):
        run["first_run_search_time"] = run["search_time_all"][0]
    return run

exp.add_report(CompareConfigsReport(
                    compared_configs=grouped_configs_to_compare(configs.configs_satisficing_ipc()),
                    attributes=common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES + ["first_run_search_time"],
                    filter=add_first_run_search_time,
                ),
                outfile="issue462-sat-compare-portfolio-configs.html"
)

exp.add_scatter_plot_step()

exp()
