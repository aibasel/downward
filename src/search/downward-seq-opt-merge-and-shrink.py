#! /usr/bin/env python
# -*- coding: utf-8 -*-

import portfolio

CONFIGS = [
    (800, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,greedy=true,group_by_h=false)))"]),
    (1000, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_linear_reverse_level,shrink_strategy=shrink_bisimulation(max_states=200000,greedy=false,group_by_h=true)))"]),
     ]

portfolio.run(CONFIGS, optimal=True)
