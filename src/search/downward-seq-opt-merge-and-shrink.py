#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import seq_opt_portfolio

CONFIGS = [
    (800, ["--search",
           "astar(mas(max_states=1,merge_strategy=5,shrink_strategy=12))"]),
    (1000, ["--search",
           "astar(mas(max_states=200000,merge_strategy=5,shrink_strategy=7))"]),
     ]

seq_opt_portfolio.run(CONFIGS)
