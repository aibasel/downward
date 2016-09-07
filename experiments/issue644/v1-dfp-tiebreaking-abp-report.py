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
        IssueConfig('dfp-reg-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=old_to_new,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-reg-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=new_to_old,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-reg-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=random,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-inv-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=old_to_new,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-inv-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=new_to_old,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-inv-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=random,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-rnd-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=old_to_new,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-rnd-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=new_to_old,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        IssueConfig('dfp-rnd-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=random,atomic_before_product=true),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-reg-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=old_to_new,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-reg-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=new_to_old,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-reg-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=regular,product_ts_order=random,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-inv-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=old_to_new,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-inv-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=new_to_old,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-inv-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=inverse,product_ts_order=random,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-rnd-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=old_to_new,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-rnd-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=new_to_old,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
        #IssueConfig('dfp-rnd-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp(atomic_ts_order=random,product_ts_order=random,atomic_before_product=false),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
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

    exp.add_fetcher('data/issue644-v1-dfp-tiebreaking-eval',
        filter_config=[
            'issue644-base-dfp-reg-otn-abp-b50k',
            'issue644-base-dfp-reg-nto-abp-b50k',
            'issue644-base-dfp-reg-rnd-abp-b50k',
            'issue644-base-dfp-inv-otn-abp-b50k',
            'issue644-base-dfp-inv-nto-abp-b50k',
            'issue644-base-dfp-inv-rnd-abp-b50k',
            'issue644-base-dfp-rnd-otn-abp-b50k',
            'issue644-base-dfp-rnd-nto-abp-b50k',
            'issue644-base-dfp-rnd-rnd-abp-b50k',
            'issue644-v1-dfp-reg-otn-abp-b50k',
            'issue644-v1-dfp-reg-nto-abp-b50k',
            'issue644-v1-dfp-reg-rnd-abp-b50k',
            'issue644-v1-dfp-inv-otn-abp-b50k',
            'issue644-v1-dfp-inv-nto-abp-b50k',
            'issue644-v1-dfp-inv-rnd-abp-b50k',
            'issue644-v1-dfp-rnd-otn-abp-b50k',
            'issue644-v1-dfp-rnd-nto-abp-b50k',
            'issue644-v1-dfp-rnd-rnd-abp-b50k',
        ])

    exp.add_comparison_table_step()

    #if matplotlib:
        #for attribute in ["memory", "total_time"]:
            #for config in configs:
                #exp.add_report(
                    #RelativeScatterPlotReport(
                        #attributes=[attribute],
                        #filter_config=["{}-{}".format(rev, config.nick) for rev in revisions],
                        #get_category=lambda run1, run2: run1.get("domain"),
                    #),
                    #outfile="{}-{}-{}.png".format(exp.name, attribute, config.nick)
                #)

    exp()

main(revisions=['issue644-base', 'issue644-v1'])
