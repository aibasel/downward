#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment
try:
    from relativescatter import RelativeScatterPlotReport
    matplotlib = True
except ImportError:
    print 'matplotlib not availabe, scatter plots not available'
    matplotlib = False

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/repos/downward/benchmarks')
    suite=suites.suite_all()

    configs = {
        IssueConfig('blind', ['--search', 'astar(blind())'], driver_options=['--search-time-limit', '60s']),
        IssueConfig('lama-first', [], driver_options=['--alias', 'lama-first', '--search-time-limit', '60s']),
    }

    exp = IssueExperiment(
        benchmarks_dir=benchmarks_dir,
        suite=suite,
        revisions=revisions,
        configs=configs,
        test_suite=['depot:p01.pddl', 'gripper:prob01.pddl'],
        processes=4,
        email='silvan.sievers@unibas.ch',
    )

    attributes = exp.DEFAULT_TABLE_ATTRIBUTES
    attributes.append('translator_*')

    exp.add_comparison_table_step()

    if matplotlib:
        for attribute in ["memory", "total_time"]:
            for config in configs:
                exp.add_report(
                    RelativeScatterPlotReport(
                        attributes=[attribute],
                        filter_config=["{}-{}".format(rev, config.nick) for rev in revisions],
                        get_category=lambda run1, run2: run1.get("domain"),
                    ),
                    outfile="{}-{}-{}.png".format(exp.name, attribute, config.nick)
                )

    exp()

main(revisions=['issue671-base', 'issue671-v1'])
