OPTIMAL = True

CONFIGS = [
    (175, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),"
           "shrink_strategy=shrink_bisimulation(greedy=true),"
           "label_reduction=exact(before_shrinking=true,before_merging=false),"
           "max_states=infinity,threshold_before_merge=1))"]),
    (432, ["--search",
           "astar(merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear(variable_order=reverse_level)),"
           "shrink_strategy=shrink_bisimulation(greedy=false),"
           "label_reduction=exact(before_shrinking=true,before_merging=false),"
           "max_states=200000))"]),
    (455, ["--evaluator",
           "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true)",
           "--search",
           "astar(lmc,lazy_evaluator=lmc)"]),
    (569, ["--search",
           "astar(lmcut())"]),
     ]
