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
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_greedy([hff, hlm], preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-23
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hff,  3)])), single(sum([g(), weight(hff, 3)]), pref_only=true)]), preferred=[hff], cost_type=one, bound=BOUND)"]),
    # uniform-09
    (1, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=0)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hLM],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # uniform-13
    (1, ["--heuristic", "hAdd=add(adapt_costs(0))", "--search", "lazy(alt([single(hAdd),single(hAdd,pref_only=true)], boost=0),preferred=[hAdd],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-03
    (1, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "eager(single(sum([g(),weight(hLM, 5)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-11
    (1, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([tiebreaking([sum([weight(g(),8),weight(hBlind,9)]),hBlind]),tiebreaking([sum([weight(g(),8),weight(hLM,9)]),hLM]),tiebreaking([sum([weight(g(),8),weight(hFF,9)]),hFF]),tiebreaking([sum([weight(g(),8),weight(hGoalCount,9)]),hGoalCount])],boost=2005),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-09
    (2, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=0)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hLM],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # uniform-typed-g-19
    (1, ["--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(sum([weight(g(), 2),weight(hFF, 3)])),single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # fdss-2014-typed-g-04
    (1, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(hcea), single(hcea, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # uniform-typed-g-06
    (2, ["--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # uniform-10
    (3, ["--heuristic", "hCg=cg(adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # cedalion-typed-g-12
    (1, ["--heuristic", "hBlind=blind()", "--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "eager(alt([type_based([g()]),single(sum([g(), weight(hBlind, 2)])), single(sum([g(), weight(hFF, 2)]))], boost=4480), preferred=[], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # cedalion-sat-01
    (1, ["--landmarks", "lmg=lm_rhw(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(1))", "--heuristic", "hBlind=blind()", "--search", "lazy(alt([single(sum([g(),weight(hBlind,2)])),single(sum([g(),weight(hBlind,2)]),pref_only=true),single(sum([g(),weight(hLM,2)])),single(sum([g(),weight(hLM,2)]),pref_only=true),single(sum([g(),weight(hFF,2)])),single(sum([g(),weight(hFF,2)]),pref_only=true)],boost=4419),preferred=[hLM],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # fdss-2014-01
    (6, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_greedy([hff, hlm], preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-09
    (1, ["--landmarks", "lmg=lm_rhw(reasonable_orders=true,only_causal_landmarks=true,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hLM=lmcount(lmg,admissible=false,pref=true,transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([weight(g(),2),weight(hBlind,3)])),single(sum([weight(g(),2),weight(hBlind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hFF,3)])),single(sum([weight(g(),2),weight(hFF,3)]),pref_only=true),single(sum([weight(g(),2),weight(hLM,3)])),single(sum([weight(g(),2),weight(hLM,3)]),pref_only=true),single(sum([weight(g(),2),weight(hAdd,3)])),single(sum([weight(g(),2),weight(hAdd,3)]),pref_only=true)],boost=2474),preferred=[hAdd],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-02
    (1, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=0),preferred=[hLM],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # uniform-03
    (7, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "eager(single(sum([g(),weight(hLM, 5)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # fdss-2014-12
    (4, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager_greedy([hcg, hlm], preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # lama-pref-True-random-True
    (1, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=True, bound=BOUND)"]),
    # uniform-18
    (16, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=0)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true),single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true)], boost=500),preferred=[hLM],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # fdss-2014-10
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hff, hlm], w=3, preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-25
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hff,  3)])), single(sum([g(), weight(hff,  3)]), pref_only=true)]),  preferred=[hff], cost_type=one, bound=BOUND)"]),
    # uniform-typed-g-16
    (1, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=1)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=1000),preferred=[hLM,hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # uniform-04
    (1, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=1)", "--heuristic", "hCg=cg(adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true),single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=1000),preferred=[hLM,hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # uniform-19
    (2, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([weight(g(), 2),weight(hFF, 3)])),single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-15
    (11, ["--heuristic", "hCg=cg(adapt_costs(2))", "--search", "lazy(alt([single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=true,cost_type=2,bound=BOUND)"]),
    # uniform-typed-g-08
    (3, ["--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg,10)]),pref_only=true)],boost=100),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-16
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(), weight(hff, 3)])), single(sum([g(), weight(hff, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-00
    (5, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hGoalCount,10)])),single(sum([g(),weight(hGoalCount,10)]),pref_only=true)],boost=2000),preferred=[hFF,hGoalCount],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # lama-pref-False-random-True-typed-ff-g
    (1, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=False, bound=BOUND)"]),
    # cedalion-sat-17
    (1, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hHMax=hmax()", "--heuristic", "hBlind=blind()", "--heuristic", "hLM=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hBlind,3)])),single(sum([g(),weight(hBlind,3)]),pref_only=true),single(sum([g(),weight(hFF,3)])),single(sum([g(),weight(hFF,3)]),pref_only=true),single(sum([g(),weight(hLM,3)])),single(sum([g(),weight(hLM,3)]),pref_only=true),single(sum([g(),weight(hHMax,3)])),single(sum([g(),weight(hHMax,3)]),pref_only=true)],boost=3052),preferred=[hFF],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-11
    (3, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([tiebreaking([sum([weight(g(),8),weight(hBlind,9)]),hBlind]),tiebreaking([sum([weight(g(),8),weight(hLM,9)]),hLM]),tiebreaking([sum([weight(g(),8),weight(hFF,9)]),hFF]),tiebreaking([sum([weight(g(),8),weight(hGoalCount,9)]),hGoalCount])],boost=2005),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-04
    (3, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_greedy([hcea, hlm], preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # uniform-14
    (2, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(0))", "--search", "lazy(alt([tiebreaking([sum([g(),weight(hLM, 10)]),hLM]),tiebreaking([sum([g(),weight(hLM, 10)]),hLM],pref_only=true),tiebreaking([sum([g(),weight(hFF, 10)]),hFF]),tiebreaking([sum([g(),weight(hFF, 10)]),hFF],pref_only=true)], boost=200),preferred=[hLM],reopen_closed=true,cost_type=2,bound=BOUND)"]),
    # uniform-typed-g-09
    (36, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=0)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hLM],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # fdss-2014-15
    (1, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy_wastar([hcg, hff], w=3, preferred=[hcg, hff], cost_type=one, bound=BOUND)"]),
    # uniform-typed-g-19
    (8, ["--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(sum([weight(g(), 2),weight(hFF, 3)])),single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # lama-pref-False-random-False
    (3, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=False, preferred_successors_first=False, bound=BOUND)"]),
    # cedalion-sat-12
    (7, ["--heuristic", "hBlind=blind()", "--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "eager(alt([single(sum([g(),weight(hBlind,2)])),single(sum([g(),weight(hFF,2)]))],boost=4480),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-04
    (2, ["--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--search", "eager(alt([tiebreaking([sum([g(),weight(hBlind,7)]),hBlind]),tiebreaking([sum([g(),weight(hHMax,7)]),hHMax]),tiebreaking([sum([g(),weight(hAdd,7)]),hAdd]),tiebreaking([sum([g(),weight(hCg,7)]),hCg])],boost=2142),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-25
    (4, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy_wastar([hff],  w=3,  preferred=[hff], cost_type=one, bound=BOUND)"]),
    # uniform-03
    (14, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "eager(single(sum([g(),weight(hLM, 5)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-01
    (7, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1)", "--heuristic", "hCea=cea(transform=adapt_costs(2))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hCea),single(hCea,pref_only=true)], boost=100),preferred=[hCea],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # cedalion-sat-15
    (1, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--heuristic", "hCg=cg(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([weight(g(),2),weight(hBlind,3)])),single(sum([weight(g(),2),weight(hBlind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hFF,3)])),single(sum([weight(g(),2),weight(hFF,3)]),pref_only=true),single(sum([weight(g(),2),weight(hCg,3)])),single(sum([weight(g(),2),weight(hCg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hGoalCount,3)])),single(sum([weight(g(),2),weight(hGoalCount,3)]),pref_only=true)],boost=3662),preferred=[hFF],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-10
    (14, ["--heuristic", "hCg=cg(adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # uniform-17
    (7, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)", "--heuristic", "hCg=cg(adapt_costs(1))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-13
    (2, ["--heuristic", "hAdd=add(transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(hAdd),single(hAdd,pref_only=true)], boost=0),preferred=[hAdd],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-14
    (3, ["--heuristic", "hFF=ff(transform=adapt_costs(2))", "--search", "lazy(alt([tiebreaking([sum([g(),hFF]),hFF]),tiebreaking([sum([g(),hFF]),hFF],pref_only=true)],boost=432),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-03
    (1, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "eager(alt([type_based([g()]),single(sum([g(),weight(hLM, 5)]))]),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-06
    (4, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-typed-g-20
    (1, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "eager_greedy([hcea], preferred=[hcea], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-12
    (16, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(hcg), single(hcg, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-01
    (4, ["--landmarks", "lmg=lm_rhw(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(1))", "--heuristic", "hBlind=blind()", "--search", "lazy(alt([single(sum([g(),weight(hBlind,2)])),single(sum([g(),weight(hBlind,2)]),pref_only=true),single(sum([g(),weight(hLM,2)])),single(sum([g(),weight(hLM,2)]),pref_only=true),single(sum([g(),weight(hFF,2)])),single(sum([g(),weight(hFF,2)]),pref_only=true)],boost=4419),preferred=[hLM],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-02
    (10, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=0),preferred=[hLM],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # cedalion-sat-04
    (10, ["--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--search", "eager(alt([tiebreaking([sum([g(),weight(hBlind,7)]),hBlind]),tiebreaking([sum([g(),weight(hHMax,7)]),hHMax]),tiebreaking([sum([g(),weight(hAdd,7)]),hAdd]),tiebreaking([sum([g(),weight(hCg,7)]),hCg])],boost=2142),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-03
    (9, ["--heuristic", "hAdd=add(transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hFF,5)]),hFF]),tiebreaking([sum([weight(g(),4),weight(hFF,5)]),hFF],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hAdd,5)]),hAdd]),tiebreaking([sum([weight(g(),4),weight(hAdd,5)]),hAdd],pref_only=true)],boost=2537),preferred=[hFF,hAdd],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-typed-g-09
    (5, ["--landmarks", "lmg=lm_rhw(reasonable_orders=true, only_causal_landmarks=true, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=false)", "--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hLM=lmcount(lmg, admissible=false, pref=true, transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(sum([weight(g(), 2), weight(hBlind, 3)])), single(sum([weight(g(), 2), weight(hBlind, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hFF, 3)])), single(sum([weight(g(), 2), weight(hFF, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hLM, 3)])), single(sum([weight(g(), 2), weight(hLM, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hAdd, 3)])), single(sum([weight(g(), 2), weight(hAdd, 3)]), pref_only=true)], boost=2474), preferred=[hAdd], reopen_closed=false, cost_type=1, bound=BOUND)"]),
    # cedalion-sat-11
    (6, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([tiebreaking([sum([weight(g(),8),weight(hBlind,9)]),hBlind]),tiebreaking([sum([weight(g(),8),weight(hLM,9)]),hLM]),tiebreaking([sum([weight(g(),8),weight(hFF,9)]),hFF]),tiebreaking([sum([weight(g(),8),weight(hGoalCount,9)]),hGoalCount])],boost=2005),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # lama-pref-False-random-True
    (3, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=False, bound=BOUND)"]),
    # uniform-02
    (1, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(1))", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=0),preferred=[hLM],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # fdss-2014-25
    (30, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy_wastar([hff],  w=3,  preferred=[hff], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-13
    (1, ["--heuristic", "hCea=cea(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--search", "lazy(alt([tiebreaking([sum([g(),weight(hBlind,10)]),hBlind]),tiebreaking([sum([g(),weight(hBlind,10)]),hBlind],pref_only=true),tiebreaking([sum([g(),weight(hFF,10)]),hFF]),tiebreaking([sum([g(),weight(hFF,10)]),hFF],pref_only=true),tiebreaking([sum([g(),weight(hCea,10)]),hCea]),tiebreaking([sum([g(),weight(hCea,10)]),hCea],pref_only=true),tiebreaking([sum([g(),weight(hGoalCount,10)]),hGoalCount]),tiebreaking([sum([g(),weight(hGoalCount,10)]),hGoalCount],pref_only=true)],boost=2222),preferred=[hCea,hGoalCount],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-04
    (17, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_greedy([hcea, hlm], preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # lama-pref-True-random-False
    (1, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=False, preferred_successors_first=True, bound=BOUND)"]),
    # uniform-08
    (8, ["--heuristic", "hCg=cg(adapt_costs(1))", "--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg,10)]),pref_only=true)],boost=100),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-13
    (5, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy_wastar([hcea, hff], w=3, preferred=[hcea, hff], cost_type=one, bound=BOUND)"]),
    # uniform-03
    (33, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,m=1)", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "eager(single(sum([g(),weight(hLM, 5)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-08
    (1, ["--landmarks", "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)", "--heuristic", "hLM=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(0))", "--search", "eager(single(sum([g(),weight(hLM,3)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-15
    (19, ["--heuristic", "hCg=cg(transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=true,cost_type=2,bound=BOUND)"]),
    # uniform-09
    (50, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1,lm_cost_type=0)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true)], boost=5000),preferred=[hLM],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-08
    (5, ["--landmarks", "lmg=lm_zg(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true)", "--heuristic", "hLM=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(0))", "--search", "eager(single(sum([g(),weight(hLM,3)])),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # cedalion-typed-g-02
    (5, ["--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hFF, 10)])), single(sum([g(), weight(hFF, 10)]), pref_only=true)], boost=2000), preferred=[hFF], reopen_closed=false, cost_type=1, bound=BOUND)"]),
    # fdss-2014-typed-g-00
    (6, ["--heuristic", "hadd=add(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(hadd), single(hadd, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hadd, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-11
    (20, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([tiebreaking([sum([weight(g(),8),weight(hBlind,9)]),hBlind]),tiebreaking([sum([weight(g(),8),weight(hLM,9)]),hLM]),tiebreaking([sum([weight(g(),8),weight(hFF,9)]),hFF]),tiebreaking([sum([weight(g(),8),weight(hGoalCount,9)]),hGoalCount])],boost=2005),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-typed-g-19
    (2, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(sum([g(), weight(hcg, 3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # uniform-14
    (8, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,m=1,lm_cost_type=2)", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(0))", "--search", "lazy(alt([tiebreaking([sum([g(),weight(hLM, 10)]),hLM]),tiebreaking([sum([g(),weight(hLM, 10)]),hLM],pref_only=true),tiebreaking([sum([g(),weight(hFF, 10)]),hFF]),tiebreaking([sum([g(),weight(hFF, 10)]),hFF],pref_only=true)], boost=200),preferred=[hLM],reopen_closed=true,cost_type=2,bound=BOUND)"]),
    # fdss-2014-11
    (1, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hcea, hlm], w=3, preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-24
    (4, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hcg,  3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true)]), preferred=[hcg], cost_type=one, bound=BOUND)"]),
    # fdss-2014-20
    (3, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "eager_greedy([hcea], preferred=[hcea], cost_type=one, bound=BOUND)"]),
    # cedalion-typed-g-07
    (9, ["--landmarks", "lmg=lm_rhw(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=true)", "--heuristic", "hHMax=hmax()", "--heuristic", "hLM=lmcount(lmg, admissible=false, pref=false, transform=adapt_costs(1))", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),tiebreaking([sum([weight(g(), 2), weight(hLM, 3)]), hLM]), tiebreaking([sum([weight(g(), 2), weight(hHMax, 3)]), hHMax]), tiebreaking([sum([weight(g(), 2), weight(hAdd, 3)]), hAdd])], boost=3002), preferred=[], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # fdss-2014-00
    (6, ["--heuristic", "hadd=add(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_greedy([hadd, hlm], preferred=[hadd, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-typed-g-10
    (1, ["--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--heuristic", "hBlind=blind()", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--search", "eager(alt([type_based([g()]),tiebreaking([sum([weight(g(), 4), weight(hBlind, 5)]), hBlind]), tiebreaking([sum([weight(g(), 4), weight(hHMax, 5)]), hHMax]), tiebreaking([sum([weight(g(), 4), weight(hCg, 5)]), hCg]), tiebreaking([sum([weight(g(), 4), weight(hGoalCount, 5)]), hGoalCount])], boost=1284), preferred=[], reopen_closed=false, cost_type=1, bound=BOUND)"]),
    # cedalion-typed-g-08
    (1, ["--landmarks", "lmg=lm_zg(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=true)", "--heuristic", "hLM=lmcount(lmg, admissible=true, pref=false, transform=adapt_costs(0))", "--search", "eager(alt([type_based([g()]),single(sum([g(), weight(hLM, 3)]))]), preferred=[], reopen_closed=true, cost_type=1, bound=BOUND)"]),
    # cedalion-sat-05
    (1, ["--heuristic", "hCea=cea(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([single(sum([g(),weight(hBlind,10)])),single(sum([g(),weight(hBlind,10)]),pref_only=true),single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hCea,10)])),single(sum([g(),weight(hCea,10)]),pref_only=true)],boost=536),preferred=[hFF],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-14
    (9, ["--heuristic", "hFF=ff(transform=adapt_costs(2))", "--search", "lazy(alt([tiebreaking([sum([g(),hFF]),hFF]),tiebreaking([sum([g(),hFF]),hFF],pref_only=true)],boost=432),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # fdss-2014-10
    (4, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hff, hlm], w=3, preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-23
    (14, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hff,  3)])), single(sum([g(), weight(hff, 3)]), pref_only=true)]), preferred=[hff], cost_type=one, bound=BOUND)"]),
    # uniform-04
    (16, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=1)", "--heuristic", "hCg=cg(adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true),single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=1000),preferred=[hLM,hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # cedalion-typed-g-15
    (5, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--heuristic", "hCg=cg(transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(sum([weight(g(), 2), weight(hBlind, 3)])), single(sum([weight(g(), 2), weight(hBlind, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hFF, 3)])), single(sum([weight(g(), 2), weight(hFF, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hCg, 3)])), single(sum([weight(g(), 2), weight(hCg, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hGoalCount, 3)])), single(sum([weight(g(), 2), weight(hGoalCount, 3)]), pref_only=true)], boost=3662), preferred=[hFF], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # uniform-11
    (69, ["--landmarks", "lmg=lm_merged([lm_rhw(),lm_hm(m=1)],only_causal_landmarks=false,disjunctive_landmarks=false,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hFF=ff(adapt_costs(0))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true),single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true)], boost=500),preferred=[hFF],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # uniform-12
    (2, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hFF, 7)])),single(sum([g(),weight(hFF, 7)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # cedalion-typed-g-11
    (1, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=false, lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM, hFF=lm_ff_syn(lmg, admissible=false, transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([type_based([g()]),tiebreaking([sum([weight(g(), 8), weight(hBlind, 9)]), hBlind]), tiebreaking([sum([weight(g(), 8), weight(hLM, 9)]), hLM]), tiebreaking([sum([weight(g(), 8), weight(hFF, 9)]), hFF]), tiebreaking([sum([weight(g(), 8), weight(hGoalCount, 9)]), hGoalCount])], boost=2005), preferred=[], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # fdss-2014-26
    (5, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "lazy_greedy([hcg], preferred=[hcg], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-03
    (4, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-10
    (4, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hff, 3)])), single(sum([g(), weight(hff, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-11
    (14, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hcea, hlm], w=3, preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-24
    (21, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hcg,  3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true)]), preferred=[hcg], cost_type=one, bound=BOUND)"]),
    # uniform-17
    (65, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)", "--heuristic", "hCg=cg(adapt_costs(1))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-05
    (6, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hadd=add(transform=adapt_costs(one))", "--search", "eager_greedy([hadd, hff], preferred=[hadd, hff], cost_type=one, bound=BOUND)"]),
    # lama-pref-True-random-False
    (6, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=False, preferred_successors_first=True, bound=BOUND)"]),
    # uniform-00
    (18, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=2)", "--heuristic", "hCea=cea(adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([single(hLM),single(hLM,pref_only=true),single(hFF),single(hFF,pref_only=true),single(hCea),single(hCea,pref_only=true)], boost=0),preferred=[hLM,hCea],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # uniform-19
    (21, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([weight(g(), 2),weight(hFF, 3)])),single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # uniform-typed-g-10
    (1, ["--heuristic", "hCg=cg(transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # lama-pref-True-random-False-typed-ff-g
    (106, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=False, preferred_successors_first=True, bound=BOUND)"]),
    # cedalion-typed-g-04
    (6, ["--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--search", "eager(alt([type_based([g()]),tiebreaking([sum([g(), weight(hBlind, 7)]), hBlind]), tiebreaking([sum([g(), weight(hHMax, 7)]), hHMax]), tiebreaking([sum([g(), weight(hAdd, 7)]), hAdd]), tiebreaking([sum([g(), weight(hCg, 7)]), hCg])], boost=2142), preferred=[], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # fdss-2014-22
    (3, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "lazy_wastar([hcea],  w=3,  preferred=[hcea], cost_type=one, bound=BOUND)"]),
    # uniform-20
    (1, ["--heuristic", "hCg=cg(adapt_costs(1))", "--search", "lazy(tiebreaking([sum([g(),weight(hCg, 2)]),hCg]),preferred=[],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-15
    (38, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--heuristic", "hCg=cg(transform=adapt_costs(0))", "--search", "lazy(alt([single(sum([weight(g(),2),weight(hBlind,3)])),single(sum([weight(g(),2),weight(hBlind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hFF,3)])),single(sum([weight(g(),2),weight(hFF,3)]),pref_only=true),single(sum([weight(g(),2),weight(hCg,3)])),single(sum([weight(g(),2),weight(hCg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hGoalCount,3)])),single(sum([weight(g(),2),weight(hGoalCount,3)]),pref_only=true)],boost=3662),preferred=[hFF],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-typed-g-18
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(sum([g(), weight(hff, 3)])), single(sum([g(), weight(hff, 3)]), pref_only=true), single(sum([g(), weight(hcea, 3)])), single(sum([g(), weight(hcea, 3)]), pref_only=true)]), preferred=[hff, hcea], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-24
    (9, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(sum([g(),  weight(hcg,  3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true)]), preferred=[hcg], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-11
    (44, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false,lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=false,transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([tiebreaking([sum([weight(g(),8),weight(hBlind,9)]),hBlind]),tiebreaking([sum([weight(g(),8),weight(hLM,9)]),hLM]),tiebreaking([sum([weight(g(),8),weight(hFF,9)]),hFF]),tiebreaking([sum([weight(g(),8),weight(hGoalCount,9)]),hGoalCount])],boost=2005),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # uniform-12
    (9, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hFF, 7)])),single(sum([g(),weight(hFF, 7)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # cedalion-typed-g-08
    (6, ["--landmarks", "lmg=lm_zg(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=true)", "--heuristic", "hLM=lmcount(lmg, admissible=true, pref=false, transform=adapt_costs(0))", "--search", "eager(alt([type_based([g()]),single(sum([g(), weight(hLM, 3)]))]), preferred=[], reopen_closed=true, cost_type=1, bound=BOUND)"]),
    # uniform-typed-g-04
    (51, ["--landmarks", "lmg=lm_rhw(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=true,lm_cost_type=1)", "--heuristic", "hCg=cg(transform=adapt_costs(2))", "--heuristic", "hLM,hFF=lm_ff_syn(lmg,admissible=true,transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(sum([g(),weight(hLM, 10)])),single(sum([g(),weight(hLM, 10)]),pref_only=true),single(sum([g(),weight(hFF, 10)])),single(sum([g(),weight(hFF, 10)]),pref_only=true),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=1000),preferred=[hLM,hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # lama-pref-True-random-True
    (14, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true)], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=True, bound=BOUND)"]),
    # cedalion-typed-g-00
    (1, ["--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hFF, 10)])), single(sum([g(), weight(hFF, 10)]), pref_only=true), single(sum([g(), weight(hGoalCount, 10)])), single(sum([g(), weight(hGoalCount, 10)]), pref_only=true)], boost=2000), preferred=[hFF, hGoalCount], reopen_closed=false, cost_type=1, bound=BOUND)"]),
    # cedalion-typed-g-14
    (1, ["--heuristic", "hFF=ff(transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),tiebreaking([sum([g(), hFF]), hFF]), tiebreaking([sum([g(), hFF]), hFF], pref_only=true)], boost=432), preferred=[hFF], reopen_closed=true, cost_type=1, bound=BOUND)"]),
    # fdss-2014-11
    (52, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hcea, hlm], w=3, preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # lama-pref-True-random-True-typed-ff-g
    (1, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=True, bound=BOUND)"]),
    # uniform-typed-g-10
    (2, ["--heuristic", "hCg=cg(transform=adapt_costs(2))", "--search", "lazy(alt([type_based([g()]),single(sum([g(),weight(hCg, 10)])),single(sum([g(),weight(hCg, 10)]),pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=2,bound=BOUND)"]),
    # cedalion-typed-g-16
    (3, ["--landmarks", "lmg=lm_zg(reasonable_orders=true, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=false)", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(0))", "--heuristic", "hHMax=hmax()", "--heuristic", "hCea=cea(transform=adapt_costs(0))", "--heuristic", "hLM=lmcount(lmg, admissible=false, pref=true, transform=adapt_costs(1))", "--search", "lazy(alt([type_based([g()]),single(sum([weight(g(), 2), weight(hLM, 3)])), single(sum([weight(g(), 2), weight(hLM, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hHMax, 3)])), single(sum([weight(g(), 2), weight(hHMax, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hCg, 3)])), single(sum([weight(g(), 2), weight(hCg, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hCea, 3)])), single(sum([weight(g(), 2), weight(hCea, 3)]), pref_only=true), single(sum([weight(g(), 2), weight(hGoalCount, 3)])), single(sum([weight(g(), 2), weight(hGoalCount, 3)]), pref_only=true)], boost=2508), preferred=[hCea, hGoalCount], reopen_closed=false, cost_type=0, bound=BOUND)"]),
    # cedalion-sat-17
    (11, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hHMax=hmax()", "--heuristic", "hBlind=blind()", "--heuristic", "hLM=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hBlind,3)])),single(sum([g(),weight(hBlind,3)]),pref_only=true),single(sum([g(),weight(hFF,3)])),single(sum([g(),weight(hFF,3)]),pref_only=true),single(sum([g(),weight(hLM,3)])),single(sum([g(),weight(hLM,3)]),pref_only=true),single(sum([g(),weight(hHMax,3)])),single(sum([g(),weight(hHMax,3)]),pref_only=true)],boost=3052),preferred=[hFF],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-typed-g-13
    (1, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hcea, 3)])), single(sum([g(), weight(hcea, 3)]), pref_only=true), single(sum([g(), weight(hff, 3)])), single(sum([g(), weight(hff, 3)]), pref_only=true)]), preferred=[hcea, hff], cost_type=one, bound=BOUND)"]),
    # fdss-2014-20
    (11, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "eager_greedy([hcea], preferred=[hcea], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-11
    (4, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hcea, 3)])), single(sum([g(), weight(hcea, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hcea, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-06
    (1, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "eager_greedy([hcg, hff], preferred=[hcg, hff], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-03
    (119, ["--heuristic", "hAdd=add(transform=adapt_costs(2))", "--heuristic", "hFF=ff(transform=adapt_costs(0))", "--search", "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hFF,5)]),hFF]),tiebreaking([sum([weight(g(),4),weight(hFF,5)]),hFF],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hAdd,5)]),hAdd]),tiebreaking([sum([weight(g(),4),weight(hAdd,5)]),hAdd],pref_only=true)],boost=2537),preferred=[hFF,hAdd],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-typed-g-21
    (58, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hcg, 3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-23
    (15, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(sum([g(),  weight(hff,  3)])), single(sum([g(), weight(hff, 3)]), pref_only=true)]), preferred=[hff], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-10
    (4, ["--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--heuristic", "hBlind=blind()", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(1))", "--search", "eager(alt([tiebreaking([sum([weight(g(),4),weight(hBlind,5)]),hBlind]),tiebreaking([sum([weight(g(),4),weight(hHMax,5)]),hHMax]),tiebreaking([sum([weight(g(),4),weight(hCg,5)]),hCg]),tiebreaking([sum([weight(g(),4),weight(hGoalCount,5)]),hGoalCount])],boost=1284),preferred=[],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # cedalion-sat-05
    (26, ["--heuristic", "hCea=cea(transform=adapt_costs(1))", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([single(sum([g(),weight(hBlind,10)])),single(sum([g(),weight(hBlind,10)]),pref_only=true),single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true),single(sum([g(),weight(hCea,10)])),single(sum([g(),weight(hCea,10)]),pref_only=true)],boost=536),preferred=[hFF],reopen_closed=false,cost_type=0,bound=BOUND)"]),
    # cedalion-sat-04
    (15, ["--heuristic", "hBlind=blind()", "--heuristic", "hAdd=add(transform=adapt_costs(0))", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hHMax=hmax()", "--search", "eager(alt([tiebreaking([sum([g(),weight(hBlind,7)]),hBlind]),tiebreaking([sum([g(),weight(hHMax,7)]),hHMax]),tiebreaking([sum([g(),weight(hAdd,7)]),hAdd]),tiebreaking([sum([g(),weight(hCg,7)]),hCg])],boost=2142),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-03
    (16, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager_greedy([hff, hlm], preferred=[hff, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-00
    (12, ["--heuristic", "hadd=add(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(hadd), single(hadd, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hadd, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-23
    (32, ["--heuristic", "hff=ff(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hff,  3)])), single(sum([g(), weight(hff, 3)]), pref_only=true)]), preferred=[hff], cost_type=one, bound=BOUND)"]),
    # lama-pref-False-random-True-typed-g
    (19, ["--heuristic", "hlm,hff=lm_ff_syn(lm_rhw(reasonable_orders=true, lm_cost_type=one), transform=adapt_costs(one))", "--search", "lazy(alt([single(hff), single(hff, pref_only=true), single(hlm), single(hlm, pref_only=true), type_based([g()])], boost=1000), preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=True, preferred_successors_first=False, bound=BOUND)"]),
    # cedalion-sat-02
    (1, ["--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hFF,10)])),single(sum([g(),weight(hFF,10)]),pref_only=true)],boost=2000),preferred=[hFF],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # uniform-19
    (149, ["--heuristic", "hFF=ff(adapt_costs(1))", "--search", "lazy(alt([single(sum([weight(g(), 2),weight(hFF, 3)])),single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true)], boost=5000),preferred=[hFF],reopen_closed=true,cost_type=1,bound=BOUND)"]),
    # fdss-2014-21
    (3, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hcg, hlm], w=3, preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-22
    (1, ["--heuristic", "hcea=cea(transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(sum([g(), weight(hcea,  3)])), single(sum([g(), weight(hcea,  3)]), pref_only=true)]),  preferred=[hcea], cost_type=one, bound=BOUND)"]),
    # fdss-2014-09
    (46, ["--heuristic", "hadd=add(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "lazy_wastar([hadd, hlm], w=3, preferred=[hadd, hlm], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-12
    (25, ["--heuristic", "hBlind=blind()", "--heuristic", "hFF=ff(transform=adapt_costs(1))", "--search", "eager(alt([single(sum([g(),weight(hBlind,2)])),single(sum([g(),weight(hFF,2)]))],boost=4480),preferred=[],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # fdss-2014-typed-g-26
    (9, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(hcg),  single(hcg,  pref_only=true)]), preferred=[hcg], cost_type=one, bound=BOUND)"]),
    # fdss-2014-typed-g-08
    (4, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hadd=add(transform=adapt_costs(one))", "--search", "lazy(alt([type_based([g()]),single(hadd), single(hadd, pref_only=true), single(hcg), single(hcg, pref_only=true)]), preferred=[hadd, hcg], cost_type=one, bound=BOUND)"]),
    # cedalion-sat-17
    (32, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false,only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=true,no_orders=false)", "--heuristic", "hFF=ff(transform=adapt_costs(2))", "--heuristic", "hHMax=hmax()", "--heuristic", "hBlind=blind()", "--heuristic", "hLM=lmcount(lmg,admissible=true,pref=false,transform=adapt_costs(1))", "--search", "lazy(alt([single(sum([g(),weight(hBlind,3)])),single(sum([g(),weight(hBlind,3)]),pref_only=true),single(sum([g(),weight(hFF,3)])),single(sum([g(),weight(hFF,3)]),pref_only=true),single(sum([g(),weight(hLM,3)])),single(sum([g(),weight(hLM,3)]),pref_only=true),single(sum([g(),weight(hHMax,3)])),single(sum([g(),weight(hHMax,3)]),pref_only=true)],boost=3052),preferred=[hFF],reopen_closed=true,cost_type=0,bound=BOUND)"]),
    # cedalion-typed-g-11
    (10, ["--landmarks", "lmg=lm_exhaust(reasonable_orders=false, only_causal_landmarks=false, disjunctive_landmarks=true, conjunctive_landmarks=true, no_orders=false, lm_cost_type=1)", "--heuristic", "hGoalCount=goalcount(transform=adapt_costs(2))", "--heuristic", "hLM, hFF=lm_ff_syn(lmg, admissible=false, transform=adapt_costs(0))", "--heuristic", "hBlind=blind()", "--search", "eager(alt([type_based([g()]),tiebreaking([sum([weight(g(), 8), weight(hBlind, 9)]), hBlind]), tiebreaking([sum([weight(g(), 8), weight(hLM, 9)]), hLM]), tiebreaking([sum([weight(g(), 8), weight(hFF, 9)]), hFF]), tiebreaking([sum([weight(g(), 8), weight(hGoalCount, 9)]), hGoalCount])], boost=2005), preferred=[], reopen_closed=true, cost_type=0, bound=BOUND)"]),
    # fdss-2014-typed-g-12
    (27, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([type_based([g()]),single(hcg), single(hcg, pref_only=true), single(hlm), single(hlm, pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # uniform-typed-g-17
    (1, ["--landmarks", "lmg=lm_hm(only_causal_landmarks=false,disjunctive_landmarks=true,conjunctive_landmarks=false,no_orders=true,m=1)", "--heuristic", "hCg=cg(transform=adapt_costs(1))", "--heuristic", "hLM=lmcount(lmg,admissible=true)", "--search", "lazy(alt([type_based([g()]),single(hLM),single(hLM,pref_only=true),single(hCg),single(hCg,pref_only=true)], boost=0),preferred=[hCg],reopen_closed=false,cost_type=1,bound=BOUND)"]),
    # fdss-2014-19
    (8, ["--heuristic", "hcg=cg(transform=adapt_costs(one))", "--heuristic", "hlm=lmcount(lm_rhw(reasonable_orders=true, lm_cost_type=2), transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(), weight(hcg, 3)])), single(sum([g(), weight(hcg, 3)]), pref_only=true), single(sum([g(), weight(hlm, 3)])), single(sum([g(), weight(hlm, 3)]), pref_only=true)]), preferred=[hcg, hlm], cost_type=one, bound=BOUND)"]),
    # fdss-2014-17
    (2, ["--heuristic", "hadd=add(transform=adapt_costs(one))", "--search", "eager(alt([single(sum([g(),  weight(hadd,  3)])), single(sum([g(),  weight(hadd, 3)]), pref_only=true)]), preferred=[hadd], cost_type=one, bound=BOUND)"]),
]
