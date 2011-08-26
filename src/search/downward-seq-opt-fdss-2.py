#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import seq_opt_portfolio

CONFIGS = [
    (1, ["--search",
           "astar(mas(max_states=1,merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(greedy=true,memory_limit=false)))"]),
    (1, ["--search",
           "astar(mas(max_states=200000,merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_dfp(enable_greedy_bisimulation)))"]),
    (1, ["--search",
           "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"]),
    (1, ["--search",
           "astar(lmcut())"]),
    (1, ["--search",
           "astar(blind())"]),
     ]

seq_opt_portfolio.run(CONFIGS)
