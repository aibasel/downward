#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup_no_benchmarks import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite = suites.suite_satisficing_with_ipc11()

    configs = {
        IssueConfig('seq_sat_lama_2011', [], driver_options=['--alias', 'seq-sat-lama-2011']),
        IssueConfig('lama_first', [], driver_options=['--alias', 'lama-first']),
        IssueConfig('ehc_lm_zhu', ['--search', 'ehc(lmcount(lm_zg()))']),
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
        nick = config.nick
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["memory"],
                filter_config=["issue416-v2-base-%s" % nick, "issue416-v2-%s" % nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue416_base_v2_memory_%s.png' % nick
        )

        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["total_time"],
                filter_config=["issue416-v2-base-%s" % nick, "issue416-v2-%s" % nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue416_base_v2_total_time_%s.png' % nick
        )

    exp()

main(revisions=['issue416-v2-base', 'issue416-v2'])
