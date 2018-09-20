#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup

CONFIGS = {
    "ehc_ff": [
        "--search", "ehc(ff())"],
    "ehc_add_pref": [
        "--heuristic", "hadd=add()", "--search", "ehc(hadd, preferred=[hadd])"],
    #"ehc_add_ff_pref": [
    #    "--search", "ehc(add(), preferred=[ff()],preferred_usage=RANK_PREFERRED_FIRST)"],
}

SUITE = downward.suites.suite_satisficing_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v6-base", "issue77-v6"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
