from .util import DRIVER_DIR


PORTFOLIO_DIR = DRIVER_DIR / "portfolios"

ALIASES = {}

ALIASES["seq-sat-fd-autotune-1"] = [
    "--search",
    "let(hff, eval_modify_costs(ff(),cost_type=one),"
    "let(hcea, cea(),"
    "let(hcg, eval_modify_costs(cg(),cost_type=plusone),"
    "let(hgc, goalcount(),"
    "let(hAdd, add(),"
    """iterated([
lazy(alt([single(sum([g(),weight(hff,10)])),
          single(sum([g(),weight(hff,10)]),pref_only=true)],
         boost=2000),
     preferred=[hff],reopen_closed=false,cost_type=one),
lazy(alt([single(sum([g(),weight(hAdd,7)])),
          single(sum([g(),weight(hAdd,7)]),pref_only=true),
          single(sum([g(),weight(hcg,7)])),
          single(sum([g(),weight(hcg,7)]),pref_only=true),
          single(sum([g(),weight(hcea,7)])),
          single(sum([g(),weight(hcea,7)]),pref_only=true),
          single(sum([g(),weight(hgc,7)])),
          single(sum([g(),weight(hgc,7)]),pref_only=true)],
         boost=1000),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=one),
lazy(alt([tiebreaking([sum([g(),weight(hAdd,3)]),hAdd]),
          tiebreaking([sum([g(),weight(hAdd,3)]),hAdd],pref_only=true),
          tiebreaking([sum([g(),weight(hcg,3)]),hcg]),
          tiebreaking([sum([g(),weight(hcg,3)]),hcg],pref_only=true),
          tiebreaking([sum([g(),weight(hcea,3)]),hcea]),
          tiebreaking([sum([g(),weight(hcea,3)]),hcea],pref_only=true),
          tiebreaking([sum([g(),weight(hgc,3)]),hgc]),
          tiebreaking([sum([g(),weight(hgc,3)]),hgc],pref_only=true)],
         boost=5000),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=normal),
eager(alt([tiebreaking([sum([g(),weight(hAdd,10)]),hAdd]),
           tiebreaking([sum([g(),weight(hAdd,10)]),hAdd],pref_only=true),
           tiebreaking([sum([g(),weight(hcg,10)]),hcg]),
           tiebreaking([sum([g(),weight(hcg,10)]),hcg],pref_only=true),
           tiebreaking([sum([g(),weight(hcea,10)]),hcea]),
           tiebreaking([sum([g(),weight(hcea,10)]),hcea],pref_only=true),
           tiebreaking([sum([g(),weight(hgc,10)]),hgc]),
           tiebreaking([sum([g(),weight(hgc,10)]),hgc],pref_only=true)],
          boost=500),
      preferred=[hcea,hgc],reopen_closed=true,cost_type=normal)
],repeat_last=true,continue_on_fail=true))))))"""]

ALIASES["seq-sat-fd-autotune-2"] = [
    "--search",
    "let(hcea, eval_modify_costs(cea(),cost_type=plusone),"
    "let(hcg, eval_modify_costs(cg(),cost_type=one),"
    "let(hgc, eval_modify_costs(goalcount(),cost_type=plusone),"
    "let(hff, ff(),"
    """iterated([
ehc(hcea,preferred=[hcea],preferred_usage=prune_by_preferred,cost_type=normal),
lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),
          single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hcg,3)])),
          single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hcea,3)])),
          single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hgc,3)])),
          single(sum([weight(g(),2),weight(hgc,3)]),pref_only=true)],
         boost=200),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=one),
lazy(alt([single(sum([g(),weight(hff,5)])),
          single(sum([g(),weight(hff,5)]),pref_only=true),
          single(sum([g(),weight(hcg,5)])),
          single(sum([g(),weight(hcg,5)]),pref_only=true),
          single(sum([g(),weight(hcea,5)])),
          single(sum([g(),weight(hcea,5)]),pref_only=true),
          single(sum([g(),weight(hgc,5)])),
          single(sum([g(),weight(hgc,5)]),pref_only=true)],
         boost=5000),
     preferred=[hcea,hgc],reopen_closed=true,cost_type=normal),
lazy(alt([single(sum([g(),weight(hff,2)])),
          single(sum([g(),weight(hff,2)]),pref_only=true),
          single(sum([g(),weight(hcg,2)])),
          single(sum([g(),weight(hcg,2)]),pref_only=true),
          single(sum([g(),weight(hcea,2)])),
          single(sum([g(),weight(hcea,2)]),pref_only=true),
          single(sum([g(),weight(hgc,2)])),
          single(sum([g(),weight(hgc,2)]),pref_only=true)],
         boost=1000),
     preferred=[hcea,hgc],reopen_closed=true,cost_type=one)
],repeat_last=true,continue_on_fail=true)))))"""]

def _get_lama(pref):
    return [
        "--search",
        "--if-unit-cost",
        f"let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref={pref}),"
        "let(hff, ff(),"
        """iterated([
            lazy_greedy([hff,hlm],preferred=[hff,hlm]),
            lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),
            lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),
            lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),
            lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)
         ],repeat_last=true,continue_on_fail=true)))""",
        "--if-non-unit-cost",
        f"let(hlm1, eval_modify_costs(landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref={pref}),cost_type=one),"
        "let(hff1, eval_modify_costs(ff(),cost_type=one),"
        f"let(hlm2, eval_modify_costs(landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref={pref}),cost_type=plusone),"
        "let(hff2, eval_modify_costs(ff(),cost_type=plusone),"
        """iterated([
            lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],
                 cost_type=one,reopen_closed=false),
            lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],
                 reopen_closed=false),
            lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),
            lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),
            lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),
            lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)
        ],repeat_last=true,continue_on_fail=true)))))""",
        # Append --always to be on the safe side if we want to append
        # additional options later.
        "--always"]

ALIASES["seq-sat-lama-2011"] = _get_lama(pref="true")
ALIASES["lama"] = _get_lama(pref="false")

ALIASES["lama-first"] = [
    "--search",
    "let(hlm, eval_modify_costs(landmark_sum(lm_factory=lm_reasonable_orders_hps(lm_rhw()),pref=false),cost_type=one),"
    "let(hff, eval_modify_costs(ff(),cost_type=one),"
    """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                               cost_type=one,reopen_closed=false)))"""]

ALIASES["seq-opt-bjolp"] = [
    "--search",
    "let(lmc, landmark_cost_partitioning(lm_reasonable_orders_hps(lm_merged([lm_rhw(),lm_hm(m=1)]))),"
    "astar(lmc,lazy_evaluator=lmc))"]

ALIASES["seq-opt-lmcut"] = [
    "--search", "astar(lmcut())"]


PORTFOLIOS = {}
for portfolio in PORTFOLIO_DIR.iterdir():
    if portfolio.name == "__pycache__":
        continue
    assert portfolio.suffix == ".py", str(portfolio)
    PORTFOLIOS[portfolio.stem.replace("_", "-")] = PORTFOLIO_DIR / portfolio


def show_aliases():
    for alias in sorted(list(ALIASES) + list(PORTFOLIOS)):
        print(alias)


def set_options_for_alias(alias_name, args):
    """
    If alias_name is an alias for a configuration, set args.search_options
    to the corresponding command-line arguments. If it is an alias for a
    portfolio, set args.portfolio to the path to the portfolio file.
    Otherwise raise KeyError.
    """
    assert not args.search_options
    assert not args.portfolio

    if alias_name in ALIASES:
        args.search_options = [x.replace(" ", "").replace("\n", "")
                               for x in ALIASES[alias_name]]
    elif alias_name in PORTFOLIOS:
        args.portfolio = PORTFOLIOS[alias_name]
    else:
        raise KeyError(alias_name)
