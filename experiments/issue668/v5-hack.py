#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.absolute import AbsoluteReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

BENCHMARKS_DIR=os.path.expanduser('~/repos/downward/benchmarks')
REVISIONS = ["issue668-v5-hack"]
CONFIGS = [
    IssueConfig('sbf-miasm-rl-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=old_to_new,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rl-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rl-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=random,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-l-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=level,product_ts_order=old_to_new,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-l-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=level,product_ts_order=new_to_old,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-l-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=level,product_ts_order=random,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rnd-otn-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=random,product_ts_order=old_to_new,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rnd-nto-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=random,product_ts_order=new_to_old,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rnd-rnd-abp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=random,product_ts_order=random,atomic_before_product=true)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rl-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=old_to_new,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rl-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rl-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=random,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-l-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=level,product_ts_order=old_to_new,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-l-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=level,product_ts_order=new_to_old,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-l-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=level,product_ts_order=random,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rnd-otn-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=random,product_ts_order=old_to_new,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rnd-nto-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=random,product_ts_order=new_to_old,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-rnd-rnd-pba-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=random,product_ts_order=random,atomic_before_product=false)])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
    IssueConfig('sbf-miasm-allrnd-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),single_random])),shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))']),
]
SUITE = DEFAULT_OPTIMAL_SUITE
ENVIRONMENT = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH"])

if is_test_run():
    SUITE = ['depot:p01.pddl', 'parcprinter-opt11-strips:p01.pddl', 'mystery:prob07.pddl']
    ENVIRONMENT = LocalEnvironment(processes=4)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)
exp.add_parser('lab_driver_parser', exp.LAB_DRIVER_PARSER)
exp.add_parser('exitcode_parser', exp.EXITCODE_PARSER)
exp.add_parser('translator_parser', exp.TRANSLATOR_PARSER)
exp.add_parser('single_search_parser', exp.SINGLE_SEARCH_PARSER)
exp.add_parser('ms_parser', 'ms-parser.py')


# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)

# m&s attributes
ms_construction_time = Attribute('ms_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_atomic_construction_time = Attribute('ms_atomic_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
ms_abstraction_constructed = Attribute('ms_abstraction_constructed', absolute=True, min_wins=False)
ms_final_size = Attribute('ms_final_size', absolute=False, min_wins=True)
ms_out_of_memory = Attribute('ms_out_of_memory', absolute=True, min_wins=True)
ms_out_of_time = Attribute('ms_out_of_time', absolute=True, min_wins=True)
search_out_of_memory = Attribute('search_out_of_memory', absolute=True, min_wins=True)
search_out_of_time = Attribute('search_out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,

    ms_construction_time,
    ms_atomic_construction_time,
    ms_abstraction_constructed,
    ms_final_size,
    ms_out_of_memory,
    ms_out_of_time,
    search_out_of_memory,
    search_out_of_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_report(AbsoluteReport(attributes=attributes,filter_algorithm=[
    '%s-sbf-miasm-rl-otn-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rl-nto-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rl-rnd-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-otn-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-nto-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-rnd-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-otn-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-nto-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-rnd-abp-b50k' % 'issue668-v5-hack',
]),outfile='issue668-v5-hack-abp.html')
exp.add_report(AbsoluteReport(attributes=attributes,filter_algorithm=[
    '%s-sbf-miasm-rl-otn-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rl-nto-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rl-rnd-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-otn-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-nto-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-rnd-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-otn-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-nto-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-rnd-pba-b50k' % 'issue668-v5-hack',
]),outfile='issue668-v5-hack-pba.html')
exp.add_report(AbsoluteReport(attributes=attributes,filter_algorithm=[
    '%s-sbf-miasm-rl-nto-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-nto-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-nto-abp-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rl-nto-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-l-nto-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-rnd-nto-pba-b50k' % 'issue668-v5-hack',
    '%s-sbf-miasm-allrnd-b50k' % 'issue668-v5-hack',
]),outfile='issue668-v5-hack-paper.html')

exp.run_steps()






