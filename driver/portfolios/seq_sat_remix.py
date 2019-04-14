"""
This is the "Fast Downward Remix" sequential portfolio that participated in the IPC 2018
satisficing and bounded-cost tracks. For more information, see the planner abstract:

Jendrik Seipp.
Fast Downward Remix.
In Ninth International Planning Competition (IPC 2018), Deterministic Part, pp. 74-76. 2018.
https://ai.dmi.unibas.ch/papers/seipp-ipc2018a.pdf

Results on the training set:

Coverage: 2058
Runtime: 1800s
Score: 2003.89
"""

OPTIMAL = False
CONFIGS = [
    # fdss-2014-01
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-23
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND)"]),
    # uniform-09
    (1, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hlm],reopen_closed=false,bound=BOUND)"]),
    # uniform-13
    (1, [
        "--evaluator",
        "hadd=add()",
        "--search",
        "lazy(alt([single(hadd),single(hadd,pref_only=true)],boost=0),preferred=[hadd],reopen_closed=true,bound=BOUND)"]),
    # uniform-03
    (1, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "eager(single(sum([g(),weight(hlm,5)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-11
    (1, [
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
    # uniform-09
    (2, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hlm],reopen_closed=false,bound=BOUND)"]),
    # uniform-typed-g-19
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-04
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hcea),single(hcea,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-06
    (2, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # uniform-10
    (3, [
        "--evaluator",
        "hcg=cg(adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # cedalion-typed-g-12
    (1, [
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hff,2)]))],boost=4480),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-01
    (1, [
        "--landmarks",
        "lmg=lm_rhw(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "lazy(alt([single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hblind,2)]),pref_only=true),single(sum([g(),weight(hlm,2)])),single(sum([g(),weight(hlm,2)]),pref_only=true),single(sum([g(),weight(hff,2)])),single(sum([g(),weight(hff,2)]),pref_only=true)],boost=4419),preferred=[hlm],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-01
    (6, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-09
    (1, [
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
    # uniform-typed-g-02
    (1, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=0),preferred=[hlm],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # uniform-03
    (7, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "eager(single(sum([g(),weight(hlm,5)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-12
    (4, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hcg,hlm],preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # lama-pref-true-random-true
    (1, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=true,bound=BOUND)"]),
    # uniform-18
    (16, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true)],boost=500),preferred=[hlm],reopen_closed=false,bound=BOUND)"]),
    # fdss-2014-10
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hff,hlm],w=3,preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-25
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-16
    (1, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=1000),preferred=[hlm,hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # uniform-04
    (1, [
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
    # uniform-19
    (2, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-15
    (11, [
        "--evaluator",
        "hcg=cg(adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=true,cost_type=plusone,bound=BOUND)"]),
    # uniform-typed-g-08
    (3, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=100),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-16
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-00
    (5, [
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hgoalcount,10)])),single(sum([g(),weight(hgoalcount,10)]),pref_only=true)],boost=2000),preferred=[hff,hgoalcount],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # lama-pref-false-random-true-typed-ff-g
    (1, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([hff,g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=false,bound=BOUND)"]),
    # cedalion-sat-17
    (1, [
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
        "lazy(alt([single(sum([g(),weight(hblind,3)])),single(sum([g(),weight(hblind,3)]),pref_only=true),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true),single(sum([g(),weight(hhmax,3)])),single(sum([g(),weight(hhmax,3)]),pref_only=true)],boost=3052),preferred=[hff],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-11
    (3, [
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
    # fdss-2014-04
    (3, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hcea,hlm],preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # uniform-14
    (2, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),weight(hlm,10)]),hlm]),tiebreaking([sum([g(),weight(hlm,10)]),hlm],pref_only=true),tiebreaking([sum([g(),weight(hff,10)]),hff]),tiebreaking([sum([g(),weight(hff,10)]),hff],pref_only=true)],boost=200),preferred=[hlm],reopen_closed=true,cost_type=plusone,bound=BOUND)"]),
    # uniform-typed-g-09
    (36, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hlm],reopen_closed=false,bound=BOUND)"]),
    # fdss-2014-15
    (1, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcg,hff],w=3,preferred=[hcg,hff],cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-19
    (8, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # lama-pref-false-random-false
    (3, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=false,bound=BOUND)"]),
    # cedalion-sat-12
    (7, [
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hff,2)]))],boost=4480),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-04
    (2, [
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
    # fdss-2014-25
    (4, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hff],w=3,preferred=[hff],cost_type=one,bound=BOUND)"]),
    # uniform-03
    (14, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "eager(single(sum([g(),weight(hlm,5)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-01
    (7, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1)",
        "--evaluator",
        "hcea=cea(transform=adapt_costs(plusone))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hcea),single(hcea,pref_only=true)],boost=100),preferred=[hcea],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # cedalion-sat-15
    (1, [
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
    (14, [
        "--evaluator",
        "hcg=cg(adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # uniform-17
    (7, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hcg=cg(adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-13
    (2, [
        "--evaluator",
        "hadd=add()",
        "--search",
        "lazy(alt([type_based([g()]),single(hadd),single(hadd,pref_only=true)],boost=0),preferred=[hadd],reopen_closed=true,bound=BOUND)"]),
    # cedalion-sat-14
    (3, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),hff]),hff]),tiebreaking([sum([g(),hff]),hff],pref_only=true)],boost=432),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-03
    (1, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hlm,5)]))]),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-06
    (4, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-20
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hcea],preferred=[hcea],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-12
    (16, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-01
    (4, [
        "--landmarks",
        "lmg=lm_rhw(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "lazy(alt([single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hblind,2)]),pref_only=true),single(sum([g(),weight(hlm,2)])),single(sum([g(),weight(hlm,2)]),pref_only=true),single(sum([g(),weight(hff,2)])),single(sum([g(),weight(hff,2)]),pref_only=true)],boost=4419),preferred=[hlm],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-02
    (10, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=0),preferred=[hlm],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # cedalion-sat-04
    (10, [
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
    (9, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff]),tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd]),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd],pref_only=true)],boost=2537),preferred=[hff,hadd],reopen_closed=true,bound=BOUND)"]),
    # cedalion-typed-g-09
    (5, [
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
        "lazy(alt([type_based([g()]),single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hlm,3)])),single(sum([weight(g(),2),weight(hlm,3)]),pref_only=true),single(sum([weight(g(),2),weight(hadd,3)])),single(sum([weight(g(),2),weight(hadd,3)]),pref_only=true)],boost=2474),preferred=[hadd],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-11
    (6, [
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
    # lama-pref-false-random-true
    (3, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=false,bound=BOUND)"]),
    # uniform-02
    (1, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=0),preferred=[hlm],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # fdss-2014-25
    (30, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hff],w=3,preferred=[hff],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-13
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(plusone))",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),weight(hblind,10)]),hblind]),tiebreaking([sum([g(),weight(hblind,10)]),hblind],pref_only=true),tiebreaking([sum([g(),weight(hff,10)]),hff]),tiebreaking([sum([g(),weight(hff,10)]),hff],pref_only=true),tiebreaking([sum([g(),weight(hcea,10)]),hcea]),tiebreaking([sum([g(),weight(hcea,10)]),hcea],pref_only=true),tiebreaking([sum([g(),weight(hgoalcount,10)]),hgoalcount]),tiebreaking([sum([g(),weight(hgoalcount,10)]),hgoalcount],pref_only=true)],boost=2222),preferred=[hcea,hgoalcount],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-04
    (17, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hcea,hlm],preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # lama-pref-true-random-false
    (1, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=true,bound=BOUND)"]),
    # uniform-08
    (8, [
        "--evaluator",
        "hcg=cg(adapt_costs(one))",
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=100),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-13
    (5, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcea,hff],w=3,preferred=[hcea,hff],cost_type=one,bound=BOUND)"]),
    # uniform-03
    (33, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "eager(single(sum([g(),weight(hlm,5)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-08
    (1, [
        "--landmarks",
        "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,pref=false)",
        "--search",
        "eager(single(sum([g(),weight(hlm,3)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-15
    (19, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=true,cost_type=plusone,bound=BOUND)"]),
    # uniform-09
    (50, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hlm],reopen_closed=false,bound=BOUND)"]),
    # cedalion-sat-08
    (5, [
        "--landmarks",
        "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,pref=false)",
        "--search",
        "eager(single(sum([g(),weight(hlm,3)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-02
    (5, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true)],boost=2000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-00
    (6, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hadd),single(hadd,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hadd,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-11
    (20, [
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
    # fdss-2014-typed-g-19
    (2, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # uniform-14
    (8, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),weight(hlm,10)]),hlm]),tiebreaking([sum([g(),weight(hlm,10)]),hlm],pref_only=true),tiebreaking([sum([g(),weight(hff,10)]),hff]),tiebreaking([sum([g(),weight(hff,10)]),hff],pref_only=true)],boost=200),preferred=[hlm],reopen_closed=true,cost_type=plusone,bound=BOUND)"]),
    # fdss-2014-11
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcea,hlm],w=3,preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-24
    (4, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true)]),preferred=[hcg],cost_type=one,bound=BOUND)"]),
    # fdss-2014-20
    (3, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hcea],preferred=[hcea],cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-07
    (9, [
        "--landmarks",
        "lmg=lm_rhw(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hhmax=hmax()",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false,pref=false,transform=adapt_costs(one))",
        "--evaluator",
        "hadd=add()",
        "--search",
        "lazy(alt([type_based([g()]),tiebreaking([sum([weight(g(),2),weight(hlm,3)]),hlm]),tiebreaking([sum([weight(g(),2),weight(hhmax,3)]),hhmax]),tiebreaking([sum([weight(g(),2),weight(hadd,3)]),hadd])],boost=3002),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-00
    (6, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hadd,hlm],preferred=[hadd,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-10
    (1, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hhmax=hmax()",
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),tiebreaking([sum([weight(g(),4),weight(hblind,5)]),hblind]),tiebreaking([sum([weight(g(),4),weight(hhmax,5)]),hhmax]),tiebreaking([sum([weight(g(),4),weight(hcg,5)]),hcg]),tiebreaking([sum([weight(g(),4),weight(hgoalcount,5)]),hgoalcount])],boost=1284),preferred=[],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-08
    (1, [
        "--landmarks",
        "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,pref=false)",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hlm,3)]))]),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-05
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--evaluator",
        "hblind=blind()",
        "--search",
        "eager(alt([single(sum([g(),weight(hblind,10)])),single(sum([g(),weight(hblind,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcea,10)])),single(sum([g(),weight(hcea,10)]),pref_only=true)],boost=536),preferred=[hff],reopen_closed=false,bound=BOUND)"]),
    # cedalion-sat-14
    (9, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([tiebreaking([sum([g(),hff]),hff]),tiebreaking([sum([g(),hff]),hff],pref_only=true)],boost=432),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-10
    (4, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hff,hlm],w=3,preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-23
    (14, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND)"]),
    # uniform-04
    (16, [
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
    # cedalion-typed-g-15
    (5, [
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
    # uniform-11
    (69, [
        "--landmarks",
        "lmg=lm_merged([lm_rhw(),lm_hm(m=1)],only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hff=ff()",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true)],boost=500),preferred=[hff],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # uniform-12
    (2, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,7)])),single(sum([g(),weight(hff,7)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-11
    (1, [
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
        "eager(alt([type_based([g()]),tiebreaking([sum([weight(g(),8),weight(hblind,9)]),hblind]),tiebreaking([sum([weight(g(),8),weight(hlm,9)]),hlm]),tiebreaking([sum([weight(g(),8),weight(hff,9)]),hff]),tiebreaking([sum([weight(g(),8),weight(hgoalcount,9)]),hgoalcount])],boost=2005),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-26
    (5, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "lazy_greedy([hcg],preferred=[hcg],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-03
    (4, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-10
    (4, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-11
    (14, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcea,hlm],w=3,preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-24
    (21, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true)]),preferred=[hcg],cost_type=one,bound=BOUND)"]),
    # uniform-17
    (65, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hcg=cg(adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-05
    (6, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hadd,hff],preferred=[hadd,hff],cost_type=one,bound=BOUND)"]),
    # lama-pref-true-random-false
    (6, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=true,bound=BOUND)"]),
    # uniform-00
    (18, [
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
    # uniform-19
    (21, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-10
    (1, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # lama-pref-true-random-false-typed-ff-g
    (106, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([hff,g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=true,bound=BOUND)"]),
    # cedalion-typed-g-04
    (6, [
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hadd=add()",
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hhmax=hmax()",
        "--search",
        "eager(alt([type_based([g()]),tiebreaking([sum([g(),weight(hblind,7)]),hblind]),tiebreaking([sum([g(),weight(hhmax,7)]),hhmax]),tiebreaking([sum([g(),weight(hadd,7)]),hadd]),tiebreaking([sum([g(),weight(hcg,7)]),hcg])],boost=2142),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-22
    (3, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcea],w=3,preferred=[hcea],cost_type=one,bound=BOUND)"]),
    # uniform-20
    (1, [
        "--evaluator",
        "hcg=cg(adapt_costs(one))",
        "--search",
        "lazy(tiebreaking([sum([g(),weight(hcg,2)]),hcg]),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # cedalion-sat-15
    (38, [
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
    # fdss-2014-typed-g-18
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hcea,3)])),single(sum([g(),weight(hcea,3)]),pref_only=true)]),preferred=[hff,hcea],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-24
    (9, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true)]),preferred=[hcg],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-11
    (44, [
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
    # uniform-12
    (9, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,7)])),single(sum([g(),weight(hff,7)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-08
    (6, [
        "--landmarks",
        "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,pref=false)",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hlm,3)]))]),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-04
    (51, [
        "--landmarks",
        "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)",
        "--evaluator",
        "hcg=cg(transform=adapt_costs(plusone))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true,transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=1000),preferred=[hlm,hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # lama-pref-true-random-true
    (14, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=true,bound=BOUND)"]),
    # cedalion-typed-g-00
    (1, [
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hgoalcount,10)])),single(sum([g(),weight(hgoalcount,10)]),pref_only=true)],boost=2000),preferred=[hff,hgoalcount],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # cedalion-typed-g-14
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),tiebreaking([sum([g(),hff]),hff]),tiebreaking([sum([g(),hff]),hff],pref_only=true)],boost=432),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-11
    (52, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcea,hlm],w=3,preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # lama-pref-true-random-true-typed-ff-g
    (1, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([hff,g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=true,bound=BOUND)"]),
    # uniform-typed-g-10
    (2, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(plusone))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)"]),
    # cedalion-typed-g-16
    (3, [
        "--landmarks",
        "lmg=lm_zg(reasonable_orders=true,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)",
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hgoalcount=goalcount()",
        "--evaluator",
        "hhmax=hmax()",
        "--evaluator",
        "hcea=cea()",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=false,pref=true,transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([weight(g(),2),weight(hlm,3)])),single(sum([weight(g(),2),weight(hlm,3)]),pref_only=true),single(sum([weight(g(),2),weight(hhmax,3)])),single(sum([weight(g(),2),weight(hhmax,3)]),pref_only=true),single(sum([weight(g(),2),weight(hcg,3)])),single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hcea,3)])),single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),single(sum([weight(g(),2),weight(hgoalcount,3)])),single(sum([weight(g(),2),weight(hgoalcount,3)]),pref_only=true)],boost=2508),preferred=[hcea,hgoalcount],reopen_closed=false,bound=BOUND)"]),
    # cedalion-sat-17
    (11, [
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
        "lazy(alt([single(sum([g(),weight(hblind,3)])),single(sum([g(),weight(hblind,3)]),pref_only=true),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true),single(sum([g(),weight(hhmax,3)])),single(sum([g(),weight(hhmax,3)]),pref_only=true)],boost=3052),preferred=[hff],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-typed-g-13
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcea,3)])),single(sum([g(),weight(hcea,3)]),pref_only=true),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hcea,hff],cost_type=one,bound=BOUND)"]),
    # fdss-2014-20
    (11, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hcea],preferred=[hcea],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-11
    (4, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcea,3)])),single(sum([g(),weight(hcea,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcea,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-06
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hcg,hff],preferred=[hcg,hff],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-03
    (119, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(plusone))",
        "--evaluator",
        "hff=ff()",
        "--search",
        "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff]),tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd]),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd],pref_only=true)],boost=2537),preferred=[hff,hadd],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-typed-g-21
    (58, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-23
    (15, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-10
    (4, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hhmax=hmax()",
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hgoalcount=goalcount(transform=adapt_costs(one))",
        "--search",
        "eager(alt([tiebreaking([sum([weight(g(),4),weight(hblind,5)]),hblind]),tiebreaking([sum([weight(g(),4),weight(hhmax,5)]),hhmax]),tiebreaking([sum([weight(g(),4),weight(hcg,5)]),hcg]),tiebreaking([sum([weight(g(),4),weight(hgoalcount,5)]),hgoalcount])],boost=1284),preferred=[],reopen_closed=false,cost_type=one,bound=BOUND)"]),
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
    # cedalion-sat-04
    (15, [
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
    # fdss-2014-03
    (16, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-00
    (12, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hadd),single(hadd,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hadd,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-23
    (32, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND)"]),
    # lama-pref-false-random-true-typed-g
    (19, [
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=false,bound=BOUND)"]),
    # cedalion-sat-02
    (1, [
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true)],boost=2000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # uniform-19
    (149, [
        "--evaluator",
        "hff=ff(adapt_costs(one))",
        "--search",
        "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND)"]),
    # fdss-2014-21
    (3, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hcg,hlm],w=3,preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-22
    (1, [
        "--evaluator",
        "hcea=cea(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcea,3)])),single(sum([g(),weight(hcea,3)]),pref_only=true)]),preferred=[hcea],cost_type=one,bound=BOUND)"]),
    # fdss-2014-09
    (46, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "lazy_wastar([hadd,hlm],w=3,preferred=[hadd,hlm],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-12
    (25, [
        "--evaluator",
        "hblind=blind()",
        "--evaluator",
        "hff=ff(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hff,2)]))],boost=4480),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-typed-g-26
    (9, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true)]),preferred=[hcg],cost_type=one,bound=BOUND)"]),
    # fdss-2014-typed-g-08
    (4, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--search",
        "lazy(alt([type_based([g()]),single(hadd),single(hadd,pref_only=true),single(hcg),single(hcg,pref_only=true)]),preferred=[hadd,hcg],cost_type=one,bound=BOUND)"]),
    # cedalion-sat-17
    (32, [
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
        "lazy(alt([single(sum([g(),weight(hblind,3)])),single(sum([g(),weight(hblind,3)]),pref_only=true),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true),single(sum([g(),weight(hhmax,3)])),single(sum([g(),weight(hhmax,3)]),pref_only=true)],boost=3052),preferred=[hff],reopen_closed=true,bound=BOUND)"]),
    # cedalion-typed-g-11
    (10, [
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
        "eager(alt([type_based([g()]),tiebreaking([sum([weight(g(),8),weight(hblind,9)]),hblind]),tiebreaking([sum([weight(g(),8),weight(hlm,9)]),hlm]),tiebreaking([sum([weight(g(),8),weight(hff,9)]),hff]),tiebreaking([sum([weight(g(),8),weight(hgoalcount,9)]),hgoalcount])],boost=2005),preferred=[],reopen_closed=true,bound=BOUND)"]),
    # fdss-2014-typed-g-12
    (27, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # uniform-typed-g-17
    (1, [
        "--landmarks",
        "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)",
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lmg,admissible=true)",
        "--search",
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)"]),
    # fdss-2014-19
    (8, [
        "--evaluator",
        "hcg=cg(transform=adapt_costs(one))",
        "--evaluator",
        "hlm=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)"]),
    # fdss-2014-17
    (2, [
        "--evaluator",
        "hadd=add(transform=adapt_costs(one))",
        "--search",
        "eager(alt([single(sum([g(),weight(hadd,3)])),single(sum([g(),weight(hadd,3)]),pref_only=true)]),preferred=[hadd],cost_type=one,bound=BOUND)"]),
]
