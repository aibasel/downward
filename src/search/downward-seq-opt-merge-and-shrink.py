#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import seq_opt_portfolio

CONFIGS = [
    (800, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true,initialize_by_h=false,group_by_h=false)))"]),
    (1000, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,initialize_by_h=true,group_by_h=true)))"]),
     ]

seq_opt_portfolio.run(CONFIGS)
