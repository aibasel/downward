#! /usr/bin/env python
# -*- coding: utf-8 -*-

import portfolio

CONFIGS = [
    (175, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true,group_by_h=false)))"]),
    (432, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,group_by_h=true)))"]),
    (455, ["--search",
           "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"]),
    (569, ["--search",
           "astar(lmcut())"]),
     ]

portfolio.run(CONFIGS, optimal=True)

