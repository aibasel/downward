#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment

def main(revisions=None):
    suite=suites.suite_optimal_strips()
    suite.extend(suites.suite_ipc14_opt_strips())

    # only DFP configs
    configs = {
        # label reduction with seed 2016
        IssueConfig('dfp-b50k-lrs2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false,random_seed=2016)))']),
        IssueConfig('dfp-ginf-lrs2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false,random_seed=2016)))']),
        IssueConfig('dfp-f50k-lrs2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_fh(max_states=50000),label_reduction=exact(before_shrinking=false,before_merging=true,random_seed=2016)))']),

        # shrink fh/rnd with seed 2016
        IssueConfig('dfp-f50ks2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_fh(max_states=50000,random_seed=2016),label_reduction=exact(before_shrinking=false,before_merging=true)))']),
        IssueConfig('dfp-rnd50ks2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_random(max_states=50000,random_seed=2016),label_reduction=exact(before_shrinking=false,before_merging=true)))']),

        # shrink fh/rnd with seed 2016 and with label reduction with seed 2016
        IssueConfig('dfp-f50ks2016-lrs2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_fh(max_states=50000,random_seed=2016),label_reduction=exact(before_shrinking=false,before_merging=true,random_seed=2016)))']),
        IssueConfig('dfp-rnd50ks2016-lrs2016', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_random(max_states=50000,random_seed=2016),label_reduction=exact(before_shrinking=false,before_merging=true,random_seed=2016)))']),
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

    exp.add_absolute_report_step()

    exp()

main(revisions=['issue645-v4'])
