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
    suite=suites.suite_optimal_strips()

    configs = {
        IssueConfig('astar-blind', ['--search', 'astar(blind())']),
        IssueConfig('astar-blind-sss', ['--search', 'astar(blind(), pruning=stubborn_sets_simple())']),
        IssueConfig('astar-blind-ssec', ['--search', 'astar(blind(), pruning=stubborn_sets_ec())']),
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

    exp.add_comparison_table_step()

    if matplotlib:
        for r1, r2 in zip(revisions, revisions[1:]) + [(revisions[0], revisions[-1])]:
            for attribute in ["memory", "total_time"]:
                for config in configs:
                    exp.add_report(
                        RelativeScatterPlotReport(
                            attributes=[attribute],
                            filter_config=["{}-{}".format(rev, config.nick) for rev in [r1, r2]],
                            get_category=lambda run1, run2: run1.get("domain"),
                        ),
                        outfile="{}-{}-{}-{}-{}.png".format(exp.name, attribute, config.nick, r1, r2)
                    )

    exp()

main(revisions=['issue629-v7-base', 'issue629-v7', 'issue629-v8', 'issue629-v9'])
