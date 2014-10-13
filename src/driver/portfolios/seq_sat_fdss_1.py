# -*- coding: utf-8 -*-

OPTIMAL = False

CONFIGS = [
    # alt_lazy_ff_cg
    (49, ["--heuristic", "hff=ff(cost_type=H_COST_TYPE)",
         "--heuristic", "hcg=cg(cost_type=H_COST_TYPE)", "--search",
         "lazy_greedy(hff,hcg,preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_ff_1
    (171, ["--heuristic", "h=ff(cost_type=H_COST_TYPE)",
          "--search",
          "lazy_greedy(h,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_lazy_cea_cg
    (27, ["--heuristic", "hcea=cea(cost_type=H_COST_TYPE)",
         "--heuristic", "hcg=cg(cost_type=H_COST_TYPE)", "--search",
         "lazy_greedy(hcea,hcg,preferred=[hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_wa3_ff_1
    (340, ["--heuristic", "h=ff(cost_type=H_COST_TYPE)",
          "--search",
          "lazy_wastar(h,w=3,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_eager_ff_cg
    (76, ["--heuristic", "hff=ff(cost_type=H_COST_TYPE)",
         "--heuristic", "hcg=cg(cost_type=H_COST_TYPE)", "--search",
         "eager_greedy(hff,hcg,preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_ff_1
    (88, ["--heuristic", "h=ff(cost_type=H_COST_TYPE)",
          "--search",
          "eager_greedy(h,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_eager_ff_add
    (90, ["--heuristic", "hff=ff(cost_type=H_COST_TYPE)",
         "--heuristic", "hadd=add(cost_type=H_COST_TYPE)", "--search",
         "eager_greedy(hff,hadd,preferred=[hff,hadd],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_cea_1
    (56, ["--heuristic", "h=cea(cost_type=H_COST_TYPE)",
          "--search",
          "lazy_greedy(h,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_eager_ff_cea_cg
    (73, ["--heuristic", "hff=ff(cost_type=H_COST_TYPE)",
          "--heuristic", "hcea=cea(cost_type=H_COST_TYPE)",
          "--heuristic", "hcg=cg(cost_type=H_COST_TYPE)",
          "--search",
          "eager_greedy([hff,hcea,hcg],preferred=[hff,hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_wa3_add_1
    (50, ["--heuristic", "h=add(cost_type=H_COST_TYPE)",
          "--search",
          "lazy_wastar(h,w=3,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_cea_1
    (84, ["--heuristic", "h=cea(cost_type=H_COST_TYPE)",
          "--search",
          "eager_greedy(h,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_wa3_add_1
    (166, ["--heuristic", "h=add(cost_type=H_COST_TYPE)",
          "--search",
          "eager(single(sum([g(),weight(h,3)])),preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_wa3_ff_1
    (87, ["--heuristic", "h=ff(cost_type=H_COST_TYPE)",
          "--search",
          "eager(single(sum([g(),weight(h,3)])),preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_wa3_cg_1
    (73, ["--heuristic", "h=cg(cost_type=H_COST_TYPE)",
         "--search",
         "lazy_wastar(h,w=3,preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_wa3_cg_1
    (89, ["--heuristic", "h=cg(cost_type=H_COST_TYPE)",
          "--search",
          "eager(single(sum([g(),weight(h,3)])),preferred=h,cost_type=S_COST_TYPE,bound=BOUND)"]),
     ]

FINAL_CONFIG = [
    "--heuristic", "h=ff(cost_type=H_COST_TYPE)",
    "--search",
    "iterated(eager(single(sum([g(),weight(h,3)])),preferred=h,cost_type=S_COST_TYPE,bound=BOUND),bound=BOUND,repeat_last=true)"]
