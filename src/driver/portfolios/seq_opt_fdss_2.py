# -*- coding: utf-8 -*-

OPTIMAL = True

CONFIGS = [
    (1, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true,group_by_h=false)))"]),
    (1, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear(variable_order=reverse_level),shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,group_by_h=true)))"]),
    (1, ["--search",
           "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"]),
    (1, ["--search",
           "astar(lmcut())"]),
    (1, ["--search",
           "astar(blind())"]),
     ]
