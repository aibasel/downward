import re

OPTIMAL = False
REGEX_SEARCH = re.compile(r".*((eager_greedy|lazy_greedy)\(.*\))\)")

CONFIGS = [
    # eager_greedy_ff
    (330, ["--search",
          "let(h, ff(transform=H_COST_TRANSFORM),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # lazy_greedy_ff
    (411, ["--search",
          "let(h, ff(transform=H_COST_TRANSFORM),"
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_greedy_cea
    (213, ["--search",
          "let(h, cea(transform=H_COST_TRANSFORM),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # lazy_greedy_cea
    (57, ["--search",
          "let(h, cea(transform=H_COST_TRANSFORM),"
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_greedy_add
    (204, ["--search",
          "let(h, add(transform=H_COST_TRANSFORM),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # eager_greedy_cg
    (208, ["--search",
          "let(h, cg(transform=H_COST_TRANSFORM),"
          "eager_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # lazy_greedy_cg
    (109, ["--search",
          "let(h, cg(transform=H_COST_TRANSFORM),"
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
    # lazy_greedy_add
    (63, ["--search",
          "let(h, add(transform=H_COST_TRANSFORM),"
          "lazy_greedy([h],preferred=[h],cost_type=S_COST_TYPE,bound=BOUND))"]),
     ]

def FINAL_CONFIG_BUILDER(successful_args):
    # This assumes that CONFIGS only contains "simple" configurations.
    new_args = list(successful_args)
    for pos, arg in enumerate(successful_args):
        if arg == "--search":
            orig_search = REGEX_SEARCH.match(successful_args[pos + 1]).group(1)
            sub_searches = []
            for weight in (5, 3, 2, 1):
                if orig_search.startswith("lazy"):
                    sub_search = \
                        "lazy_wastar([h],preferred=[h],w=%d,cost_type=S_COST_TYPE)" % weight
                else:
                    sub_search = \
                        "eager(single(sum([g(),weight(h,%d)])),preferred=[h],cost_type=S_COST_TYPE)" % weight
                sub_searches.append(sub_search)
            sub_search_string = ",".join(sub_searches)
            iterated_search = "iterated([%s],bound=BOUND,repeat_last=true)" % sub_search_string
            new_search = successful_args[pos + 1].replace(orig_search, iterated_search)
            new_args[pos + 1] = new_search
            break
    return new_args
