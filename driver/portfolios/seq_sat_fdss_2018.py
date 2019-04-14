# -*- coding: utf-8 -*-
"""
This is the "Fast Downward Stone Soup 2018" sequential portfolio that participated in the IPC 2018
satisficing and bounded-cost tracks. For more information, see the planner abstract:

Jendrik Seipp and Gabriele Röger.
Fast Downward Stone Soup 2018.
In Ninth International Planning Competition (IPC 2018), Deterministic Part, pp. 80-82. 2018.
https://ai.dmi.unibas.ch/papers/seipp-roeger-ipc2018.pdf

Results on the training set:

Coverage: 2057
Granularity: 30s
Runtime: 1583s
Score: 1999.93
"""

OPTIMAL = False
CONFIGS = [
    # lama-pref-false-random-true-typed-ff-g
    (26, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([hff,g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=false,bound=BOUND)"]),
    # uniform-typed-g-02
    (25, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=0),preferred=[hlm],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # lama-pref-true-random-false
    (135, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=true,bound=BOUND)"]),
    # fdss-2014-03
    (59, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # lama-pref-true-random-true
    (23, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=true,bound=BOUND)"]),
    # uniform-04
    (57, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hcg=cg(adapt_costs(plusone))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=1000),preferred=[hlm,hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # fdss-2014-04
    (17, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hcea,hlm],preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-00
    (12, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hadd),single(hadd,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hadd,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-02
    (26, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true)],boost=2000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-12
    (28, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # uniform-00
    (29, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hcea=cea(adapt_costs(plusone))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true),single(hcea),single(hcea,pref_only=true)],boost=0),preferred=[hlm,hcea],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # fdss-2014-11
    (88, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcea,hlm],w=3,preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # uniform-08
    (8, [
        "--evaluator",
        "hcg=cg(adapt_costs(one))",
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=100),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-00
    (54, [
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hgoalcount,10)])),single(sum([g(),weight(hgoalcount,10)]),pref_only=true)],boost=2000),preferred=[hff,hgoalcount],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-16
    (24, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-01
    (29, [
        "--landmarks",
        "lmg=lm_rhw(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hblind,2)]),pref_only=true),single(sum([g(),weight(hlm,2)])),single(sum([g(),weight(hlm,2)]),pref_only=true),single(sum([g(),weight(hff,2)])),single(sum([g(),weight(hff,2)]),pref_only=true)],boost=4419),preferred=[hlm],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-25
    (30, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hff],w=3,preferred=[hff],cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-15
    (28, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=true,cost_type=plusone,bound=BOUND)"]),
    # fdss-2014-typed-g-21
    (58, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-05
    (26, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "eager(alt([single(sum([g(),weight(hblind,10)])),single(sum([g(),weight(hblind,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcea,10)])),single(sum([g(),weight(hcea,10)]),pref_only=true)],boost=536),preferred=[hff],reopen_closed=false,bound=BOUND)"]),
    # fdss-2014-20
    (27, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hcea],preferred=[hcea],cost_type=one,bound=BOUND)"]),
    # fdss-2014-23
    (50, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-15
    (28, [
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hcg=cg()",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hcg,3)])),single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hgoalcount,3)])),single(sum([weight(g(),2),weight(hgoalcount,3)]),pref_only=true)],boost=3662),preferred=[hff],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-15
    (29, [
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hcg=cg()",
        "--search",
        "lazy(alt([single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hcg,3)])),single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hgoalcount,3)])),single(sum([weight(g(),2),weight(hgoalcount,3)]),pref_only=true)],boost=3662),preferred=[hff],reopen_closed=true,bound=BOUND)"]),
    # uniform-10
    (21, [
        "--evaluator",
        "hcg=cg(adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # fdss-2014-24
    (21, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true)]),preferred=[hcg],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-09
    (24, [
        "--landmarks",
        "lmg=lm_rhw(reasonable_orders=true,only_causal_landmarks=true,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hadd=add()",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false,pref=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hlm,3)])),single(sum([weight(g(),2),weight(hlm,3)]),pref_only=true),single(sum([weight(g(),2),weight(hadd,3)])),single(sum([weight(g(),2),weight(hadd,3)]),pref_only=true)],boost=2474),preferred=[hadd],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-04
    (28, [
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hadd=add()",
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hhmax=hmax()",
        "--search",
        "eager(alt([tiebreaking([sum([g(),weight(hblind,7)]),hblind]),tiebreaking([sum([g(),weight(hhmax,7)]),hhmax]),tiebreaking([sum([g(),weight(hadd,7)]),hadd]),tiebreaking([sum([g(),weight(hcg,7)]),hcg])],boost=2142),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-03
    (28, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff]),tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd]),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd],pref_only=true)],boost=2537),preferred=[hff,hadd],reopen_closed=true,bound=BOUND)"]),
    # uniform-typed-g-09
    (53, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hlm],reopen_closed=false,bound=BOUND)"]),
    # uniform-19
    (29, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-12
    (27, [
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hff,2)]))],boost=4480),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # uniform-typed-g-16
    (29, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=1000),preferred=[hlm,hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # uniform-14
    (54, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),weight(hlm,10)]),hlm]),tiebreaking([sum([g(),weight(hlm,10)]),hlm],pref_only=true),tiebreaking([sum([g(),weight(hff,10)]),hff]),tiebreaking([sum([g(),weight(hff,10)]),hff],pref_only=true)],boost=200),preferred=[hlm],reopen_closed=true,cost_type=plusone,bound=BOUND)"]),
    # uniform-17
    (87, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hcg=cg(adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-17
    (30, [
        "--landmarks",
        "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--evaluator",
        "hhmax=hmax()",
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hblind,3)])),single(sum([g(),weight(hblind,3)]),pref_only=true),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true),single(sum([g(),weight(hhmax,3)])),single(sum([g(),weight(hhmax,3)]),pref_only=true)],boost=3052),preferred=[hff],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-14
    (56, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),hff]),hff]),tiebreaking([sum([g(),hff]),hff],pref_only=true)],boost=432),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-11
    (19, [
        "--landmarks",
        "lmg=lm_merged([lm_rhw(),lm_hm(m=1)],only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hff=ff()",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true)],boost=500),preferred=[hff],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # cedalion-sat-11
    (56, [
        "--landmarks",
        "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(plusone))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false)",
        "--evaluator",
        "hff=ff()",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "eager(alt([tiebreaking([sum([weight(g(),8),weight(hblind,9)]),hblind]),tiebreaking([sum([weight(g(),8),weight(hlm,9)]),hlm]),tiebreaking([sum([weight(g(),8),weight(hff,9)]),hff]),tiebreaking([sum([weight(g(),8),weight(hgoalcount,9)]),hgoalcount])],boost=2005),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-08
    (24, [
        "--landmarks",
        "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,pref=false)",
        "--search",
        "eager(single(sum([g(),weight(hlm,3)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-03
    (81, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "eager(single(sum([g(),weight(hlm,5)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
]

