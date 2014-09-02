#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites, configs

import common_setup

exp = common_setup.IssueExperiment(
    search_revisions=["issue462-base", "issue462-v1"],
    configs=configs.default_configs_satisficing(),
    suite=suites.suite_satisficing_with_ipc11(),
    limits={"search_time": 30}
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
