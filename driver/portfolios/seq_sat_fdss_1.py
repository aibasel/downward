# -*- coding: utf-8 -*-

OPTIMAL = False

CONFIGS = [
    # alt_lazy_ff_cg
    (49, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
         "--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)", "--search",
         "lazy_greedy([hff,hcg],preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_ff_1
    (171, ["--evaluator", "h=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_lazy_cea_cg
    (27, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)",
         "--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)", "--search",
         "lazy_greedy([hcea,hcg],preferred=[hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_wa3_ff_1
    (340, ["--evaluator", "h=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_eager_ff_cg
    (76, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
         "--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)", "--search",
         "eager_greedy([hff,hcg],preferred=[hff,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_ff_1
    (88, ["--evaluator", "h=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_eager_ff_add
    (90, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
         "--evaluator", "hadd=add(transform=H_COST_TRANSFORM)", "--search",
         "eager_greedy([hff,hadd],preferred=[hff,hadd],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_cea_1
    (56, ["--evaluator", "h=cea(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # alt_eager_ff_cea_cg
    (73, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
          "--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)",
          "--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([hff,hcea,hcg],preferred=[hff,hcea,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_wa3_add_1
    (50, ["--evaluator", "h=add(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_cea_1
    (84, ["--evaluator", "h=cea(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_wa3_add_1
    (166, ["--evaluator", "h=add(transform=H_COST_TRANSFORM)",
          "--search",
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_wa3_ff_1
    (87, ["--evaluator", "h=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_wa3_cg_1
    (73, ["--evaluator", "h=cg(transform=H_COST_TRANSFORM)",
         "--search",
         "lazy_wastar([h],w=3,preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_wa3_cg_1
    (89, ["--evaluator", "h=cg(transform=H_COST_TRANSFORM)",
          "--search",
          "eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
     ]

FINAL_CONFIG = [
    "--evaluator", "h=ff(transform=H_COST_TRANSFORM)",
    "--search",
    "iterated([eager(single(sum([g(),weight(h,3)])),preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)],bound=BOUND,repeat_last=true)"]
