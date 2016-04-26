#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment
from relativescatter import RelativeScatterPlotReport

def main(revisions=None):
    suite=suites.suite_optimal_strips()
    suite.extend(suites.suite_ipc14_opt_strips())

    configs = {
        IssueConfig('dfp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-ginf', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-f50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_fh(max_states=50000),label_reduction=exact(before_shrinking=false,before_merging=true)))']),
    }

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        suite=suite,
        test_suite=['depot:pfile1'],
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

    exp.add_comparison_table_step()

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

main(revisions=['issue645-v1', 'issue645-v2'])
