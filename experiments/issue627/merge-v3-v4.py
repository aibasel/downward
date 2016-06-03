#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup_no_benchmarks import IssueConfig, IssueExperiment, get_script_dir
from relativescatter import RelativeScatterPlotReport

import os

def main(revisions=None):
    exp = IssueExperiment(benchmarks_dir=".", suite=[])

    exp.add_fetcher(
        os.path.join(get_script_dir(), "data", "issue627-v3-eval"),
        filter=lambda(run): "base" not in run["config"],
    )
    exp.add_fetcher(
        os.path.join(get_script_dir(), "data", "issue627-v4-eval"),
        filter=lambda(run): "base" not in run["config"],
    )

    for config_nick in ['astar-blind', 'astar-lmcut', 'astar-ipdb', 'astar-cegar-original', 'astar-cegar-lm-goals']:
        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["memory"],
                filter_config=["issue627-v3-%s" % config_nick,
                               "issue627-v4-%s" % config_nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_v3_v4_memory_%s.png' % config_nick
        )

        exp.add_report(
            RelativeScatterPlotReport(
                attributes=["total_time"],
                filter_config=["issue627-v3-%s" % config_nick,
                               "issue627-v4-%s" % config_nick],
                get_category=lambda run1, run2: run1.get("domain"),
            ),
            outfile='issue627_v3_v4_total_time_%s.png' % config_nick
        )

    exp()

main(revisions=['issue627-v3', 'issue627-v4'])
