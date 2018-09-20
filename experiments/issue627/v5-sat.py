#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup_no_benchmarks import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_satisficing_with_ipc11()

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
        benchmarks_dir="/infai/pommeren/projects/downward/benchmarks/",
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=['depot:pfile1'],
        processes=4,
        email='florian.pommerening@unibas.ch',
    )

    exp.add_comparison_table_step()

    for config in configs:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["memory"],
                filter_config=["issue627-v3-base-%s" % config.nick,
                               "issue627-v5-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_base_v5_sat_memory_%s.png' % config.nick
        )

        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["total_time"],
                filter_config=["issue627-v3-base-%s" % config.nick,
                               "issue627-v5-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_base_v5_sat_total_time_%s.png' % config.nick
        )

    exp()

main(revisions=['issue627-v3-base', 'issue627-v5'])
