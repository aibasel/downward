# -*- coding: utf-8 -*-

OPTIMAL = False

CONFIGS = [
    # eager_greedy_ff
    (330, ["--heuristic", "h=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_ff
    (411, ["--heuristic", "h=ff(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_cea
    (213, ["--heuristic", "h=cea(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_cea
    (57, ["--heuristic", "h=cea(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_add
    (204, ["--heuristic", "h=add(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # eager_greedy_cg
    (208, ["--heuristic", "h=cg(transform=H_COST_TRANSFORM)",
          "--search",
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_cg
    (109, ["--heuristic", "h=cg(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
    # lazy_greedy_add
    (63, ["--heuristic", "h=add(transform=H_COST_TRANSFORM)",
          "--search",
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND)"]),
     ]

def FINAL_CONFIG_BUILDER(successful_args):
# this heavily depends on that in CONFIGS only "simple" configurations are used
    new_args = list(successful_args)
    for pos, arg in enumerate(successful_args):
        if arg == "--search":
            orig_search = successful_args[pos + 1]
            sub_searches = []
            for weight in (5,3,2,1):
                if orig_search.startswith("lazy"):
                    sub_search = \
                        "lazy_wastar([h],preferred=[h],w=%d,cost_type=S_COST_TYPE)" % weight
                else:
                    sub_search = \
                        "eager(single(sum([g(),weight(h,%d)])),preferred=[h],cost_type=S_COST_TYPE)" % weight
                sub_searches.append(sub_search)
            sub_search_string = ",".join(sub_searches)
            new_search = "iterated([%s],bound=BOUND,repeat_last=true)" % sub_search_string
            new_args[pos + 1] = new_search
            break
    return new_args
