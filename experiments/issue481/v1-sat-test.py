#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue481-base", "issue481-v1"]
SUITE = suites.suite_satisficing_with_ipc11()

CONFIGS = [
    IssueConfig("ff", ["--search", "eager_greedy(ff())"]),
    # example using driver options:
    # IssueConfig("lama", [], driver_options=["--alias", "seq-sat-lama-2011"]),
]

# TODO: Add email notification? Needs a change to common_setup.py
# where the MaiaEnvironment is constructed. See:
#
# https://lab.readthedocs.org/en/latest/lab.experiment.html#lab.environments.MaiaEnvironment

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="malte.helmert@unibas.ch"
)

## TODO: Pick which steps we actually want; probably not all three.
## scatter plots commented out for now while we don't have a usable matplotlib.
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
