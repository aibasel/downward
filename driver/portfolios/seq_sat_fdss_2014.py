OPTIMAL = False

CONFIGS = [
    # add_lm_lazy_greedy
    (114, ["--evaluator", "hadd=add(transform=H_COST_TRANSFORM)",
         "--evaluator",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "lazy_greedy([hadd,hlm],preferred=[hadd,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # ff_lm_lazy_greedy
    (187, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
         "--evaluator",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "lazy_greedy([hff,hlm],preferred=[hff,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # add_lm_eager_greedy
    (33, ["--evaluator", "hadd=add(transform=H_COST_TRANSFORM)",
         "--evaluator",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "eager_greedy([hadd,hlm],preferred=[hadd,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # ff_lm_eager_greedy
    (35, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
         "--evaluator",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "eager_greedy([hff,hlm],preferred=[hff,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_lm_lazy_greedy
    (39, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)",
         "--evaluator",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "lazy_greedy([hcea,hlm],preferred=[hcea,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # add_ff_eager_greedy
    (120, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
          "--evaluator", "hadd=add(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([hadd,hff],preferred=[hadd,hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_ff_eager_greedy
    (40, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
         "--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)", "--search",
         "eager_greedy([hcg,hff],preferred=[hcg,hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # add_ff_lazy_greedy
    (17, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
          "--evaluator", "hadd=add(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([hadd,hff],preferred=[hadd,hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # add_cg_lazy_greedy
    (40, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)",
          "--evaluator", "hadd=add(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([hadd,hcg],preferred=[hadd,hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # add_lm_lazy_wastar
    (79, ["--evaluator", "hadd=add(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
          "lazy_wastar([hadd,hlm],w=3,preferred=[hadd,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # ff_lm_lazy_wastar
    (159, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
          "lazy_wastar([hff,hlm],w=3,preferred=[hff,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_lm_lazy_wastar
    (39, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
          "lazy_wastar([hcea,hlm],w=3,preferred=[hcea,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_lm_eager_greedy
    (78, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)",
         "--evaluator",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "eager_greedy([hcg,hlm],preferred=[hcg,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_ff_lazy_wastar
    (39, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)",
          "--evaluator", "hff=ff(transform=H_COST_TRANSFORM)", "--search",
          "lazy_wastar([hcea,hff],w=3,preferred=[hcea,hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_lm_eager_wastar
    (37, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "eager(alt([single(sum([g(), weight(hcea, 3)])),single(sum([g(),weight(hcea,3)]),pref_only=true),single(sum([g(), weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcea,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_ff_lazy_wastar
    (40, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)",
          "--evaluator", "hff=ff(transform=H_COST_TRANSFORM)", "--search",
          "lazy_wastar([hcg,hff],w=3,preferred=[hcg,hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # ff_lm_eager_wastar
    (40, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hff,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # add_eager_wastar
    (77, ["--evaluator", "hadd=add(transform=H_COST_TRANSFORM)",
          "--search",
          "eager(alt([single(sum([g(), weight(hadd, 3)])),single(sum([g(), weight(hadd,3)]),pref_only=true)]),preferred=[hadd],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_ff_eager_wastar
    (40, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)", "--heuristic",
         "hcea=cea(transform=H_COST_TRANSFORM)", "--search",
         "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hcea,3)])),single(sum([g(),weight(hcea,3)]),pref_only=true)]),preferred=[hff,hcea],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_lm_eager_wastar
    (78, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
         "eager(alt([single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcg,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_eager_greedy
    (40, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)",
         "--search",
         "eager_greedy([hcea],preferred=[hcea],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_lm_lazy_wastar
    (39, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)", "--heuristic",
         "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))", "--search",
          "lazy_wastar([hcg,hlm],w=3,preferred=[hcg,hlm],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cea_lazy_wastar
    (40, ["--evaluator", "hcea=cea(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_wastar([hcea], w=3, preferred=[hcea],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # ff_eager_wastar
    (72, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "eager(alt([single(sum([g(), weight(hff, 3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_eager_wastar
    (38, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)",
          "--search",
          "eager(alt([single(sum([g(), weight(hcg, 3)])),single(sum([g(),weight(hcg,3)]),pref_only=true)]),preferred=[hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # ff_lazy_wastar
    (38, ["--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_wastar([hff], w=3, preferred=[hff],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # cg_lazy_greedy
    (116, ["--evaluator", "hcg=cg(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([hcg],preferred=[hcg],cost_type=S_COST_TYPE,bound=BOUND)"]),
     ]

# ff_lm_eager_wastar
FINAL_CONFIG = [
    "--evaluator", "hff=ff(transform=H_COST_TRANSFORM)",
    "--evaluator",
    "hlm=lmcount(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone))",
    "--search",
    "iterated([eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hff,hlm],cost_type=S_COST_TYPE,bound=BOUND)],bound=BOUND,repeat_last=true)"]
