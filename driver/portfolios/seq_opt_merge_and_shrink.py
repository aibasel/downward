# -*- coding: utf-8 -*-

OPTIMAL = True

CONFIGS = [
    (800, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),"
           "shrink_strategy=shrink_bisimulation(greedy=true),"
           "label_reduction=exact(before_shrinking=true,before_merging=false),"
           "max_states=infinity,threshold_before_merge=1))"]),
    (1000, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),"
           "shrink_strategy=shrink_bisimulation(greedy=false),"
           "label_reduction=exact(before_shrinking=true,before_merging=false),"
           "max_states=200000))"]),
     ]
