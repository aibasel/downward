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

    configs = {
        IssueConfig('dfp-reg-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=old_to_new,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-reg-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=new_to_old,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-reg-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=random,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-inv-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=old_to_new,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-inv-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=new_to_old,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-inv-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=random,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-rnd-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=old_to_new,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-rnd-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=new_to_old,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-rnd-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=random,atomic_before_product=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-reg-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=old_to_new,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-reg-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=new_to_old,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-reg-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=random,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-inv-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=old_to_new,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-inv-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=new_to_old,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-inv-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=random,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-rnd-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=old_to_new,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-rnd-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=new_to_old,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-rnd-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=random,atomic_before_product=false),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
        IssueConfig('dfp-allrnd-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(randomized_order=true),shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
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

    exp.add_absolute_report_step()

    exp()

main(revisions=['issue656-v1'])
