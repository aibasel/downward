#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute, gm

from common_setup import IssueConfig, IssueExperiment

def main(revisions=None):
    suite = suites.suite_optimal_with_ipc11()

    configs = {
        IssueConfig('rl-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        #IssueConfig('cggl-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=cg_goal_level),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        #IssueConfig('rl-ginf', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('cggl-ginf', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=cg_goal_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        #IssueConfig('dfp-ginf', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        #IssueConfig('rl-f50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_fh(max_states=50000),label_reduction=exact(before_shrinking=false,before_merging=true)))']),
        #IssueConfig('cggl-f50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=cg_goal_level),shrink_strategy=shrink_fh(max_states=50000),label_reduction=exact(before_shrinking=false,before_merging=true)))']),
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
    search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
    search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)
    perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)
    proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)

    # m&s attributes
    ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[gm])
    ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
    ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
    ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
    ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
    ms_memory_delta = Attribute('ms_memory_delta', absolute=False, min_wins=True)

    extra_attributes = [
        search_out_of_memory,
        search_out_of_time,
        perfect_heuristic,
        proved_unsolvability,

        ms_construction_time,
        ms_abstraction_constructed,
        ms_final_size,
        ms_out_of_memory,
        ms_out_of_time,
        ms_memory_delta,
    ]
    attributes = exp.DEFAULT_TABLE_ATTRIBUTES
    attributes.extend(extra_attributes)

    exp.add_comparison_table_step()

    exp()

main(revisions=['issue604-v5', 'issue604-v6'])
