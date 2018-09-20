#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup_no_benchmarks import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
        IssueConfig('astar-blind', ['--search', 'astar(blind())']),
        IssueConfig('astar-lmcut', ['--search', 'astar(lmcut())']),
        IssueConfig('astar-ipdb', ['--search', 'astar(ipdb())']),
        IssueConfig('astar-cegar-original', ['--search', 'astar(cegar(subtasks=[original()], max_states=10000, max_time=infinity))']),
        IssueConfig('astar-cegar-lm-goals', ['--search', 'astar(cegar(subtasks=[landmarks(),goals()], max_states=10000, max_time=infinity))']),
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
                               "issue627-v3-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_base_v3_memory_%s.png' % config.nick
        )

        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["total_time"],
                filter_config=["issue627-v3-base-%s" % config.nick,
                               "issue627-v3-%s" % config.nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_base_v3_total_time_%s.png' % config.nick
        )

    exp()

main(revisions=['issue627-v3-base', 'issue627-v3'])
