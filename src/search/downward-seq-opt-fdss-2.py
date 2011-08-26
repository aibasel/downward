#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import seq_opt_portfolio

CONFIGS = [
    (1, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true,initialize_by_h=false,group_by_h=false)))"]),
    (1, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,initialize_by_h=true,group_by_h=true)))"]),
    (1, ["--search",
           "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"]),
    (1, ["--search",
           "astar(lmcut())"]),
    (1, ["--search",
           "astar(blind())"]),
     ]

seq_opt_portfolio.run(CONFIGS)
