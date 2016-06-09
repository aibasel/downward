#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
        IssueConfig('astar-cegar-original-10000', ['--search', 'astar(cegar(subtasks=[original()],max_states=10000,max_time=infinity))']),
        IssueConfig('astar-cegar-lm-goals-10000', ['--search', 'astar(cegar(subtasks=[landmarks(),goals()],max_states=10000,max_time=infinity))']),
    }

    exp = IssueExperiment(
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
                filter_config=["issue627-base-%s" % config.nick,
                               "4ed2abfab4ba-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_base_memory_%s.png' % config.nick
        )

        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["total_time"],
                filter_config=["issue627-base-%s" % config.nick,
                               "4ed2abfab4ba-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_base_total_time_%s.png' % config.nick
        )

    exp()

main(revisions=['issue627-base', '4ed2abfab4ba'])
