#! /usr/bin/env python
# -*- coding: utf-8 -*-

import common_setup
import downward.suites

# This experiment only tests the Lama-FF synergy, which sat1 did not
# test because it did not work in the issue77 branch.
CONFIGS = {
    "synergy":
        ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true))",
         "--search", "eager_greedy([hff,hlm],preferred=[hff,hlm])"],
    }

SUITE = downward.suites.suite_satisficing_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-v3-base", "issue77-v3"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
exp.add_scatter_plot_step()

exp()
