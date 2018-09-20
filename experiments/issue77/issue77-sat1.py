#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.configs
import downward.suites

CONFIGS = downward.configs.default_configs_satisficing(extended=True)

# The following lines remove some configs that we don't currently
# support because the respective configurations are commented out

DISABLED = [
    "seq_sat_fdss_1",
    "seq_sat_fdss_2",
    "seq_sat_lama_2011",
]
for key, value in list(CONFIGS.items()):
    if key in DISABLED or key.startswith(("lazy", "iterated", "ehc")):
        del CONFIGS[key]
    else:
        for pos, arg in enumerate(value):
            if ", pathmax=false" in arg:
                # pathmax is gone in this branch
                value[pos] = arg.replace(", pathmax=false", "")
print(sorted(CONFIGS.keys()))
print(len(CONFIGS))


SUITE = downward.suites.suite_satisficing_with_ipc11()

import common_setup

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-base", "issue77-v1"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
