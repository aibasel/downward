#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites

from common_setup import IssueConfig, IssueExperiment


REVS = ["issue648-base", "issue648-v1"]
SUITE=suites.suite_optimal_strips()
SUITE.extend(suites.suite_ipc14_opt_strips())

CONFIGS = [
    # Test label reduction, shrink_bucket_based (via shrink_fh and shrink_random)
    IssueConfig('dfp-b50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),label_reduction=exact(before_shrinking=true,before_merging=false)))']),
    IssueConfig('dfp-f50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_fh(max_states=50000),label_reduction=exact(before_shrinking=false,before_merging=true)))']),
    IssueConfig('dfp-r50k', ['--search', 'astar(merge_and_shrink(merge_strategy=merge_dfp,shrink_strategy=shrink_random(max_states=50000),label_reduction=exact(before_shrinking=false,before_merging=true)))']),
    # Test sampling
    IssueConfig('ipdb', ['--search', 'astar(ipdb)']),
    # Test genetic pattern generation
    IssueConfig('genetic', ['--search', 'astar(zopdbs(patterns=genetic))']),
    # Test cegar
    IssueConfig(
        "cegar-10K-goals-randomorder",
        ["--search", "astar(cegar(subtasks=[goals(order=random)],max_states=10000,max_time=infinity))"]),
    IssueConfig(
        "cegar-10K-original-randomorder",
        ["--search", "astar(cegar(subtasks=[original],max_states=10000,max_time=infinity,pick=random))"]),
]

exp = IssueExperiment(
    revisions=REVS,
    configs=CONFIGS,
    suite=SUITE,
    email="silvan.sievers@unibas.ch"
)

exp.add_comparison_table_step()

exp()
