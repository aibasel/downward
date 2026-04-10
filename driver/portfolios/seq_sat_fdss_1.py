OPTIMAL = False

CONFIGS = [
    # alt_lazy_ff_cg
    (49, ["--search",
          "let(hff, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
          "let(hcg, eval_modify_costs(cg(),cost_type=H_COST_TYPE),"
          "lazy_greedy([hff,hcg],preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # lazy_greedy_ff_1
    (171, ["--search",
           "let(h, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
           "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_lazy_cea_cg
    (27, ["--search",
          "let(hcea, eval_modify_costs(cea(),cost_type=H_COST_TYPE),"
          "let(hcg, eval_modify_costs(cg(),cost_type=H_COST_TYPE),"
          "lazy_greedy([hcea,hcg],preferred=[hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # lazy_wa3_ff_1
    (340, ["--search",
           "let(h, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
           "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_eager_ff_cg
    (76, ["--search",
          "let(hff, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
          "let(hcg, eval_modify_costs(cg(),cost_type=H_COST_TYPE),"
          "eager_greedy([hff,hcg],preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # eager_greedy_ff_1
    (88, ["--search",
          "let(h, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_eager_ff_add
    (90, ["--search",
          "let(hff, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
          "let(hadd, eval_modify_costs(add(),cost_type=H_COST_TYPE),"
          "eager_greedy([hff,hadd],preferred=[hff,hadd],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # lazy_greedy_cea_1
    (56, ["--search",
          "let(h, eval_modify_costs(cea(),cost_type=H_COST_TYPE),"
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_eager_ff_cea_cg
    (73, ["--search",
          "let(hff, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
          "let(hcea, eval_modify_costs(cea(),cost_type=H_COST_TYPE),"
          "let(hcg, eval_modify_costs(cg(),cost_type=H_COST_TYPE),"
          "eager_greedy([hff,hcea,hcg],preferred=[hff,hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND))))"]),
    # lazy_wa3_add_1
    (50, ["--search",
          "let(h, eval_modify_costs(add(),cost_type=H_COST_TYPE),"
          "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_greedy_cea_1
    (84, ["--search",
          "let(h, eval_modify_costs(cea(),cost_type=H_COST_TYPE),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_wa3_add_1
    (166, ["--search",
           "let(h, eval_modify_costs(add(),cost_type=H_COST_TYPE),"
           "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_wa3_ff_1
    (87, ["--search",
          "let(h, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # lazy_wa3_cg_1
    (73, ["--search",
          "let(h, eval_modify_costs(cg(),cost_type=H_COST_TYPE),"
          "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_wa3_cg_1
    (89, ["--search",
          "let(h, eval_modify_costs(cg(),cost_type=H_COST_TYPE),"
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
     ]

FINAL_CONFIG = [
    "--search",
    "let(h, eval_modify_costs(ff(),cost_type=H_COST_TYPE),"
    "iterated([eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)],bound=BOUND,repeat_last=true))"]
