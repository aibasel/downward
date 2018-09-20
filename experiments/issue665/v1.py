#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

import os

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
        IssueConfig('astar-blind', ['--search', 'astar(blind())'],
                    driver_options=['--search-time-limit', '5m']),
    }

    exp = IssueExperiment(
        benchmarks_dir=os.path.expanduser('~/projects/downward/benchmarks'),
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=['depot:pfile1'],
        processes=4,
        email='florian.pommerening@unibas.ch',
    )

    exp.add_comparison_table_step()

    attribute = "total_time"
    config_nick = 'astar-blind'
    exp.add_report(
        RelativeScatterPlotReport(
            attributes=[attribute],
            filter_config=["{}-{}".format(rev, config_nick) for rev in revisions],
            get_category=lambda run1, run2: run1.get("domain"),
        ),
        outfile="{}-{}-{}.png".format(exp.name, attribute, config_nick)
    )

    exp()

main(revisions=['issue665-base', 'issue665-v1'])
