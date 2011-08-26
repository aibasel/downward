#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import seq_opt_portfolio

CONFIGS = [
    (800, ["--search",
           "astar(mas(max_states=1,merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(greedy=true,memory_limit=false)))"]),
    (1000, ["--search",
           "astar(mas(max_states=200000,merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_dfp(enable_greedy_bisimulation)))"]),
     ]

seq_opt_portfolio.run(CONFIGS)
