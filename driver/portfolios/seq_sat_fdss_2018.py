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
    # lama-pref-False-random-True-typed-ff-g
    (26, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=False, bound=BOUND)"]),
    # uniform-typed-g-02
    (25, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=0),preferred=[hLM],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # lama-pref-True-random-False
    (135, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=False, preferred_successors_first=True, bound=BOUND)"]),
    # fdss-2014-03
    (59, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager_greedy([hff, hlm], preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # lama-pref-True-random-True
    (23, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=True, bound=BOUND)"]),
    # uniform-04
    (57, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=1)", "--heuristic", "hCg=cg(adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true),single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=1000),preferred=[hLM,hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # fdss-2014-04
    (17, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_greedy([hcea, hlm], preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-00
    (12, ["--heuristic", "hadd=add(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(hadd), single(hadd, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hadd, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-02
    (26, ["--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true)],boost=2000),preferred=[hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-typed-g-12
    (28, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(hcg), single(hcg, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # uniform-00
    (29, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=2)", "--heuristic", "hCea=cea(adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true),single(hCea),single(hCea,pref_only=true)], boost=0),preferred=[hLM,hCea],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # fdss-2014-11
    (88, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hcea, hlm], w=3, preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # uniform-08
    (8, ["--heuristic", "hCg=cg(adapt_costs(1))", "--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg,10)]),pref_only=true)],boost=100),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-00
    (54, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hGoalCount,10)])),single(sum([g(),weight(hGoalCount,10)]),pref_only=true)],boost=2000),preferred=[hFF,hGoalCount],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-typed-g-16
    (24, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(sum([g(), weight(hff, 3)])), single(sum([g(), weight(hff, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-typed-g-01
    (29, ["--landmarks", "lmg=lm_rhw(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=false, conjunctive_landmarks=true, no_orders=false, lm_cost_type=2)", "--heuristic", "hLM, hFF=lm_ff_syn(lmg, admissible=false, transform=adapt_costs(1))", "--heuristic", "hBlind=blind()", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hBlind, 2)])), single(sum([g(), weight(hBlind, 2)]), pref_only=true), single(sum([g(), weight(hLM, 2)])), single(sum([g(), weight(hLM, 2)]), pref_only=true), single(sum([g(), weight(hFF, 2)])), single(sum([g(), weight(hFF, 2)]), pref_only=true)], boost=4419), preferred=[hLM], reopen_closed=true, cost_type=1, bound=BOUND)"]),
    # fdss-2014-25
    (30, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy_wastar([hff],  w=3,  preferred=[hff], cost_type=one, bound=BOUND)"]),
    # uniform-typed-g-15
    (28, ["--heuristic", "hCg=cg(transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=true,cost_type=2,bound=BOUND)"]),
    # fdss-2014-typed-g-21
    (58, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hcg, 3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-05
    (26, ["--heuristic", "hCea=cea(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([single(sum([g(),weight(hBlind,10)])),single(sum([g(),weight(hBlind,10)]),pref_only=true),single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hCea,10)])),single(sum([g(),weight(hCea,10)]),pref_only=true)],boost=536),preferred=[hFF],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # fdss-2014-20
    (27, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "eager_greedy([hcea], preferred=[hcea], cost_type=one, bound=BOUND)"]),
    # fdss-2014-23
    (50, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hff,  3)])), single(sum([g(), weight(hff, 3)]), pref_only=true)]), preferred=[hff], cost_type=one, bound=BOUND)"]),
    # cedalion-typed-g-15
    (28, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--heuristic", "hCg=cg(transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(sum([weight(g(), 2), weight(hBlind, 3)])), single(sum([weight(g(), 2), weight(hBlind, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hFF, 3)])), single(sum([weight(g(), 2), weight(hFF, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hCg, 3)])), single(sum([weight(g(), 2), weight(hCg, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hGoalCount, 3)])), single(sum([weight(g(), 2), weight(hGoalCount, 3)]), pref_only=true)], boost=3662), preferred=[hFF], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # cedalion-sat-15
    (29, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--heuristic", "hCg=cg(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([weight(g(),2),weight(hBlind,3)])),single(sum([weight(g(),2),weight(hBlind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hFF,3)])),single(sum([weight(g(),2),weight(hFF,3)]),pref_only=true),single(sum([weight(g(),2),weight(hCg,3)])),single(sum([weight(g(),2),weight(hCg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hGoalCount,3)])),single(sum([weight(g(),2),weight(hGoalCount,3)]),pref_only=true)],boost=3662),preferred=[hFF],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-10
    (21, ["--heuristic", "hCg=cg(adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # fdss-2014-24
    (21, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hcg,  3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true)]), preferred=[hcg], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-09
    (24, ["--landmarks", "lmg=lm_rhw(reasonable_orders=true,only_causal_landmarks=true,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hLM=lmcount(lmg,admissible=false,pref=true,transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([weight(g(),2),weight(hBlind,3)])),single(sum([weight(g(),2),weight(hBlind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hFF,3)])),single(sum([weight(g(),2),weight(hFF,3)]),pref_only=true),single(sum([weight(g(),2),weight(hLM,3)])),single(sum([weight(g(),2),weight(hLM,3)]),pref_only=true),single(sum([weight(g(),2),weight(hAdd,3)])),single(sum([weight(g(),2),weight(hAdd,3)]),pref_only=true)],boost=2474),preferred=[hAdd],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-04
    (28, ["--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--search", "eager(alt([tiebreaking([sum([g(),weight(hBlind,7)]),hBlind]),tiebreaking([sum([g(),weight(hHMax,7)]),hHMax]),tiebreaking([sum([g(),weight(hAdd,7)]),hAdd]),tiebreaking([sum([g(),weight(hCg,7)]),hCg])],boost=2142),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-03
    (28, ["--heuristic", "hAdd=add(transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hFF,5)]),hFF]),tiebreaking([sum([weight(g(),4),weight(hFF,5)]),hFF],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hAdd,5)]),hAdd]),tiebreaking([sum([weight(g(),4),weight(hAdd,5)]),hAdd],pref_only=true)],boost=2537),preferred=[hFF,hAdd],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-typed-g-09
    (53, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=0)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hLM],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # uniform-19
    (29, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([weight(g(), 2),weight(hFF, 3)])),single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-12
    (27, ["--heuristic", "hBlind=blind()", "--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "eager(alt([single(sum([g(),weight(hBlind,2)])),single(sum([g(),weight(hFF,2)]))],boost=4480),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-typed-g-16
    (29, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=1)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=1000),preferred=[hLM,hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # uniform-14
    (54, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(0))", "--search", "lazy(alt([tiebreaking([sum([g(),weight(hLM, 10)]),hLM]),tiebreaking([sum([g(),weight(hLM, 10)]),hLM],pref_only=true),tiebreaking([sum([g(),weight(hFF, 10)]),hFF]),tiebreaking([sum([g(),weight(hFF, 10)]),hFF],pref_only=true)], boost=200),preferred=[hLM],reopen_closed=true,cost_type=2,bound=BOUND)"]),
    # uniform-17
    (87, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)", "--heuristic", "hCg=cg(adapt_costs(1))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # cedalion-typed-g-17
    (30, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=false)", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hHMax=hmax()", "--heuristic", "hBlind=blind()", "--heuristic", "hLM=lmcount(lmg, admissible=true, pref=false, transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hBlind, 3)])), single(sum([g(), weight(hBlind, 3)]), pref_only=true), single(sum([g(), weight(hFF, 3)])), single(sum([g(), weight(hFF, 3)]), pref_only=true), single(sum([g(), weight(hLM, 3)])), single(sum([g(), weight(hLM, 3)]), pref_only=true), single(sum([g(), weight(hHMax, 3)])), single(sum([g(), weight(hHMax, 3)]), pref_only=true)], boost=3052), preferred=[hFF], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # cedalion-sat-14
    (56, ["--heuristic", "hFF=ff(transform=adapt_costs(2))", "--search", "lazy(alt([tiebreaking([sum([g(),hFF]),hFF]),tiebreaking([sum([g(),hFF]),hFF],pref_only=true)],boost=432),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-11
    (19, ["--landmarks", "lmg=lm_merged([lm_rhw(),lm_hm(m=1)],only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hFF=ff(adapt_costs(0))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true),single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true)], boost=500),preferred=[hFF],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # cedalion-sat-11
    (56, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([tiebreaking([sum([weight(g(),8),weight(hBlind,9)]),hBlind]),tiebreaking([sum([weight(g(),8),weight(hLM,9)]),hLM]),tiebreaking([sum([weight(g(),8),weight(hFF,9)]),hFF]),tiebreaking([sum([weight(g(),8),weight(hGoalCount,9)]),hGoalCount])],boost=2005),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-08
    (24, ["--landmarks", "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)", "--heuristic", "hLM=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(0))", "--search", "eager(single(sum([g(),weight(hLM,3)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-03
    (81, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "eager(single(sum([g(),weight(hLM, 5)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
]

