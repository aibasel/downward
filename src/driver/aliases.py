# -*- coding: utf-8 -*-

SIMPLE_ALIASES = {}
COMPLEX_ALIASES_UNIT_COST = {}
COMPLEX_ALIASES_GENERAL_COST = {}


SIMPLE_ALIASES["seq-sat-fd-autotune-1"] = [
    "--heuristic", "hff=ff(cost_type=unit)",
    "--heuristic", "hcea=cea(cost_type=normal)",
    "--heuristic", "hcg=cg(cost_type=plusone)",
    "--heuristic", "hgc=goalcount(cost_type=normal)",
    "--heuristic", "hAdd=add(cost_type=normal)",
    "--search", """iterated([
lazy(alt([single(sum([g(),weight(hff,10)])),
          single(sum([g(),weight(hff,10)]),pref_only=true)],
         boost=2000),
     preferred=hff,reopen_closed=false,cost_type=unit),
lazy(alt([single(sum([g(),weight(hAdd,7)])),
          single(sum([g(),weight(hAdd,7)]),pref_only=true),
          single(sum([g(),weight(hcg,7)])),
          single(sum([g(),weight(hcg,7)]),pref_only=true),
          single(sum([g(),weight(hcea,7)])),
          single(sum([g(),weight(hcea,7)]),pref_only=true),
          single(sum([g(),weight(hgc,7)])),
          single(sum([g(),weight(hgc,7)]),pref_only=true)],
         boost=1000),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=unit),
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
      preferred=[hcea,hgc],reopen_closed=true,pathmax=true,cost_type=normal)
],repeat_last=true,continue_on_fail=true)"""]

SIMPLE_ALIASES["seq-sat-fd-autotune-2"] = [
    "--heuristic", "hcea=cea(cost_type=plusone)",
    "--heuristic", "hcg=cg(cost_type=unit)",
    "--heuristic", "hgc=goalcount(cost_type=plusone)",
    "--heuristic", "hff=ff(cost_type=normal)",
    "--search", """iterated([
ehc(hcea,preferred=hcea,preferred_usage=0,cost_type=normal),
lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),
          single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hcg,3)])),
          single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hcea,3)])),
          single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hgc,3)])),
          single(sum([weight(g(),2),weight(hgc,3)]),pref_only=true)],
         boost=200),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=unit),
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
     preferred=[hcea,hgc],reopen_closed=true,cost_type=unit)
],repeat_last=true,continue_on_fail=true)"""]

COMPLEX_ALIASES_UNIT_COST["seq-sat-lama-2011"] = [
    "--heuristic",
    "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true,"
    "                         lm_cost_type=plusone,cost_type=plusone))",
    "--search", """iterated([
                     lazy_greedy([hff,hlm],preferred=[hff,hlm]),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)
                     ],repeat_last=true,continue_on_fail=true)"""]

COMPLEX_ALIASES_GENERAL_COST["seq-sat-lama-2011"] = [
    "--heuristic",
    "hlm1,hff1=lm_ff_syn(lm_rhw(reasonable_orders=true,"
    "                           lm_cost_type=unit,cost_type=unit))",
    "--heuristic",
    "hlm2,hff2=lm_ff_syn(lm_rhw(reasonable_orders=true,"
    "                           lm_cost_type=plusone,cost_type=plusone))",
    "--search", """iterated([
                     lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],
                                 cost_type=unit,reopen_closed=false),
                     lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],
                                 reopen_closed=false),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)
                     ],repeat_last=true,continue_on_fail=true)"""]

SIMPLE_ALIASES["seq-opt-fd-autotune"] = [
    "--heuristic", "hlmcut=lmcut()",
    "--heuristic", "hhmax=hmax()",
    "--heuristic" "hselmax=selmax([hlmcut,hhmax],alpha=4,classifier=0,"
    "                             conf_threshold=0.85,training_set=10,"
    "                             sample=0,uniform=true)",
    "--search", "astar(hselmax,mpd=false,pathmax=true,cost_type=normal)"]

SIMPLE_ALIASES["seq-opt-selmax"] = [
    "--search",
    "astar(selmax([lmcut(),"
    "              lmcount(lm_merged([lm_hm(m=1),lm_rhw()]),admissible=true)"
    "             ],training_set=1000),mpd=true)"]

SIMPLE_ALIASES["seq-opt-bjolp"] = [
    "--search",
    "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),"
    "      mpd=true)"]

SIMPLE_ALIASES["seq-opt-lmcut"] = [
    "--search", "astar(lmcut())"]


assert set(COMPLEX_ALIASES_UNIT_COST) == set(COMPLEX_ALIASES_GENERAL_COST)
assert not(set(SIMPLE_ALIASES) & set(COMPLEX_ALIASES_UNIT_COST))


def show_aliases():
    aliases = list(SIMPLE_ALIASES) + list(COMPLEX_ALIASES_GENERAL_COST)
    for alias in sorted(aliases):
        print alias


def set_args_for_alias(alias_name, args):
    """If alias_name is a simple alias, set args.search_args to the
    command-line arguments. If it is a complex alias, set
    args.search_args to the command-line arguments for the
    general-cost case and args.unit_cost_search_args to the
    command-line arguments for the unit-cost case."""

    # TODO: Implement and document effect on args.portfolio.

    assert not args.search_args

    args.search_args = SIMPLE_ALIASES.get(alias_name)
    if args.search_args is not None:
        return

    args.search_args = COMPLEX_ALIASES_GENERAL_COST.get(alias_name)
    if args.search_args is not None:
        args.unit_cost_search_args = COMPLEX_ALIASES_UNIT_COST[alias_name]
        return

    raise NotImplementedError("portfolio aliases not implemented")
    # TODO: The old code checked if a portfolio file with the given name
    # existed and then loaded it. How should we implement this now?
