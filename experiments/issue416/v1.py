#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
        IssueConfig('astar-blind', ['--search', 'astar(blind())']),
        IssueConfig('astar-lmcut', ['--search', 'astar(lmcut())']),
        IssueConfig('astar-ipdb', ['--search', 'astar(ipdb())']),
        IssueConfig('astar-seq_opt_bjolp', ['--search', 'astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true), mpd=true)']),
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
        nick = config.nick
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["memory"],
                filter_config=["issue416-base-%s" % nick, "issue416-v1-%s" % nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue416_base_v1_memory_%s.png' % nick
        )
    
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["total_time"],
                filter_config=["issue416-base-%s" % nick, "issue416-v1-%s" % nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue416_base_v1_total_time_%s.png' % nick
        )

    exp()

main(revisions=['issue416-base', 'issue416-v1'])
