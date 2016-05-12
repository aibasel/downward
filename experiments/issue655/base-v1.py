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
    suite=suites.suite_optimal_strips()

    # dummy configs with correct names so that comparison report works
    configs = {
        IssueConfig('rl-b50k', []),
        IssueConfig('cggl-b50k', []),
        IssueConfig('dfp-b50k', []),
        IssueConfig('rl-ginf', []),
        IssueConfig('cggl-ginf', []),
        IssueConfig('dfp-ginf', []),
        IssueConfig('rl-f50k', []),
        IssueConfig('cggl-f50k', []),
        IssueConfig('dfp-f50k', []),
    }

    exp = IssueExperiment(
        benchmarks_dir=benchmarks_dir,
        suite=suite,
        revisions=revisions,
        configs=configs,
        test_suite=['depot:p01.pddl'],
        processes=4,
        email='silvan.sievers@unibas.ch',
    )
    exp.add_resource('ms_parser', 'ms-parser.py', dest='ms-parser.py')
    exp.add_command('ms-parser', ['ms_parser'])

    # planner outcome attributes
    perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)
    proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)
    actual_search_time = Attribute('actual_search_time', absolute=False, min_wins=True, functions=[gm])

    # m&s attributes
    ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[gm])
    ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
    ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
    ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
    ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
    search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
    search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)

    extra_attributes = [
        perfect_heuristic,
        proved_unsolvability,
        actual_search_time,

        ms_construction_time,
        ms_abstraction_constructed,
        ms_final_size,
        ms_out_of_memory,
        ms_out_of_time,
        search_out_of_memory,
        search_out_of_time,
    ]
    attributes = exp.DEFAULT_TABLE_ATTRIBUTES
    attributes.extend(extra_attributes)

    exp.add_fetcher('data/issue655-base-eval')
    exp.add_fetcher('data/issue655-v1-eval')

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

main(revisions=['issue655-base', 'issue655-v1'])
