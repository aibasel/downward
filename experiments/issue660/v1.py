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
    print 'matplotlib not available, scatter plots not available'
    matplotlib = False

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/projects/downward/benchmarks')
    suite=suites.suite_satisficing()

    configs = {
        IssueConfig('lazy-greedy-ff', [
            '--heuristic',
            'h=ff()',
            '--search',
            'lazy_greedy(h, preferred=h)'
        ]),
        IssueConfig('lama-first', [],
            driver_options=['--alias', 'lama-first']
        ),
        IssueConfig('eager_greedy_cg', [
            '--heuristic',
            'h=cg()',
            '--search',
            'eager_greedy(h, preferred=h)'
        ]),
        IssueConfig('eager_greedy_cea', [
            '--heuristic',
            'h=cea()',
            '--search',
            'eager_greedy(h, preferred=h)'
        ]),
    }

    exp = IssueExperiment(
        benchmarks_dir=benchmarks_dir,
        suite=suite,
        revisions=revisions,
        configs=configs,
        test_suite=['depot:p01.pddl'],
        processes=4,
        email='florian.pommerening@unibas.ch',
    )

    exp.add_comparison_table_step(
        filter_domain=[
            'assembly',
            'miconic-fulladl',
            'openstacks',
            'openstacks-sat08-adl',
            'optical-telegraphs',
            'philosophers',
            'psr-large',
            'psr-middle',
            'trucks',
        ],
    )

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

main(revisions=['issue660-base', 'issue660-v1'])
