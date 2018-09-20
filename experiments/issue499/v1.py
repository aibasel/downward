#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
        IssueConfig('astar-lmcut', ['--search', 'astar(lmcut())']),
    }

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=['depot:pfile1'],
        processes=4,
        email='martin.wehrle@unibas.ch',
    )

    exp.add_comparison_table_step()
    
    exp.add_report(
        RelativeScatterPlotReport(
            attributes=["memory"],
            filter_config=["issue499-base-astar-lmcut", "issue499-v1-astar-lmcut"],
            get_category=lambda run1, run2: run1.get("domain"),
        ),
        outfile='issue499_base_v1_memory.png'
    )

    exp.add_report(
        RelativeScatterPlotReport(
            attributes=["total_time"],
            filter_config=["issue499-base-astar-lmcut", "issue499-v1-astar-lmcut"],
            get_category=lambda run1, run2: run1.get("domain"),
        ),
        outfile='issue499_base_v1_total_time.png'
    )

    exp.add_report(
        RelativeScatterPlotReport(
            attributes=["expansions_until_last_jump"],
            filter_config=["issue499-base-astar-lmcut", "issue499-v1-astar-lmcut"],
            get_category=lambda run1, run2: run1.get("domain"),
        ),
        outfile='issue499_base_v1_expansions_until_last_jump.png'
    )
    
    exp()

main(revisions=['issue499-base', 'issue499-v1'])
