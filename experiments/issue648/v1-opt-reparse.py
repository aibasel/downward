#! /usr/bin/env python
# -*- coding: utf-8 -*-

from downward import suites
from lab.reports import Attribute
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

exp.add_fetcher('data/issue648-v1-opt-test', parsers=['parser.py'])

# planner outcome attributes
perfect_heuristic = Attribute('perfect_heuristic', absolute=True, min_wins=False)
proved_unsolvability = Attribute('proved_unsolvability', absolute=True, min_wins=False)
out_of_memory = Attribute('out_of_memory', absolute=True, min_wins=True)
out_of_time = Attribute('out_of_time', absolute=True, min_wins=True)

extra_attributes = [
    perfect_heuristic,
    proved_unsolvability,
    out_of_memory,
    out_of_time,
]
attributes = exp.DEFAULT_TABLE_ATTRIBUTES
attributes.extend(extra_attributes)

exp.add_comparison_table_step(attributes=attributes)

exp()
