OPTIMAL = False

CONFIGS = [
    # alt_lazy_ff_cg
    (49, ["--search",
          "let(hff, ff(transform=H_COST_TRANSFORM),"
          "let(hcg, cg(transform=H_COST_TRANSFORM),"
          "lazy_greedy([hff,hcg],preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # lazy_greedy_ff_1
    (171, ["--search",
           "let(h, ff(transform=H_COST_TRANSFORM),"
           "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_lazy_cea_cg
    (27, ["--search",
          "let(hcea, cea(transform=H_COST_TRANSFORM),"
          "let(hcg, cg(transform=H_COST_TRANSFORM),"
          "lazy_greedy([hcea,hcg],preferred=[hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # lazy_wa3_ff_1
    (340, ["--search",
           "let(h, ff(transform=H_COST_TRANSFORM),"
           "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_eager_ff_cg
    (76, ["--search",
          "let(hff, ff(transform=H_COST_TRANSFORM),"
          "let(hcg, cg(transform=H_COST_TRANSFORM),"
          "eager_greedy([hff,hcg],preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # eager_greedy_ff_1
    (88, ["--search",
          "let(h, ff(transform=H_COST_TRANSFORM),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_eager_ff_add
    (90, ["--search",
          "let(hff, ff(transform=H_COST_TRANSFORM),"
          "let(hadd, add(transform=H_COST_TRANSFORM),"
          "eager_greedy([hff,hadd],preferred=[hff,hadd],cost_type=S_COST_TYPE,bound=BOUND)))"]),
    # lazy_greedy_cea_1
    (56, ["--search",
          "let(h, cea(transform=H_COST_TRANSFORM),"
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # alt_eager_ff_cea_cg
    (73, ["--search",
          "let(hff, ff(transform=H_COST_TRANSFORM),"
          "let(hcea, cea(transform=H_COST_TRANSFORM),"
          "let(hcg, cg(transform=H_COST_TRANSFORM),"
          "eager_greedy([hff,hcea,hcg],preferred=[hff,hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND))))"]),
    # lazy_wa3_add_1
    (50, ["--search",
          "let(h, add(transform=H_COST_TRANSFORM),"
          "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_greedy_cea_1
    (84, ["--search",
          "let(h, cea(transform=H_COST_TRANSFORM),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_wa3_add_1
    (166, ["--search",
           "let(h, add(transform=H_COST_TRANSFORM),"
           "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_wa3_ff_1
    (87, ["--search",
          "let(h, ff(transform=H_COST_TRANSFORM),"
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # lazy_wa3_cg_1
    (73, ["--search",
          "let(h, cg(transform=H_COST_TRANSFORM),"
          "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_wa3_cg_1
    (89, ["--search",
          "let(h, cg(transform=H_COST_TRANSFORM),"
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
     ]

FINAL_CONFIG = [
    "--search",
    "let(h, ff(transform=H_COST_TRANSFORM),"
    "iterated([eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)],bound=BOUND,repeat_last=true))"]
