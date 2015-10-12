# -*- coding: utf-8 -*-

OPTIMAL = True

CONFIGS = [
    (175, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))"]),
    (432, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false),label_reduction=label_reduction(before_shrinking=true,before_merging=false)))"]),
    (455, ["--search",
           "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"]),
    (569, ["--search",
           "astar(lmcut())"]),
     ]
