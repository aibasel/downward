#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import seq_opt_portfolio

CONFIGS = [
    (175, ["--search",
           "astar(mas(max_states=1,merge_strategy=5,shrink_strategy=12))"]),
    (432, ["--search",
           "astar(mas(max_states=200000,merge_strategy=5,shrink_strategy=7))"]),
    (455, ["--search",
           "astar(lmcount(lm_merged(lm_rhw(),lm_hm(m=1)),admissible=true),mpd=true)"]),
    (569, ["--search",
           "astar(lmcut())"]),
     ]

seq_opt_portfolio.run(CONFIGS)

