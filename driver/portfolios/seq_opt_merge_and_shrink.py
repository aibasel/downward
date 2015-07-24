# -*- coding: utf-8 -*-

OPTIMAL = True

CONFIGS = [
    (800, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))"]),
    (1000, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))"]),
     ]
