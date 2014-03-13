#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward.suites import suite_satisficing_with_ipc11
from downward.configs import default_configs_satisficing
from downward.reports.scatter import ScatterPlotReport

import common_setup


REVS = ["issue214-base", "issue214-v2"]
CONFIGS = default_configs_satisficing()

TEST_RUN = True

if TEST_RUN:
    SUITE = "gripper:prob01.pddl"
    PRIORITY = None  # "None" means local experiment
else:
    SUITE = suite_satisficing_with_ipc11()
    PRIORITY = 0     # number means maia experiment


exp = common_setup.MyExperiment(
    grid_priority=PRIORITY,
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    parsers=['state_size_parser.py'],
    )


exp.add_comparison_table_step(
    attributes=common_setup.MyExperiment.DEFAULT_TABLE_ATTRIBUTES + ['bytes_per_state', 'variables', 'state_var_t_size']
)
exp.add_scatter_plot_step()

exp.add_report(ScatterPlotReport(
        attributes=['bytes_per_state'],
        filter_config_nick='astar_blind',
    ),
    outfile='issue214_sat_bytes_per_state.png')

for config_nick in ['lazy_greedy_ff', 'eager_greedy_cg', 'seq_sat_lama_2011']:
    for attr in ['memory', 'total_time']:
        exp.add_report(ScatterPlotReport(
                attributes=[attr],
                filter_config_nick=config_nick,
            ),
            outfile='issue214_sat_%s_%s.png' % (attr, config_nick))


exp()
