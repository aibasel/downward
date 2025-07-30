"""
This is the "Fast Downward Stone Soup 2023" sequential portfolio that
participated in the IPC 2023 agile and satisficing tracks.

Clemens Büchner, Remo Christen, Augusto Blaas Corrêa, Salomé Eriksson, Patrick Ferber, Jendrik Seipp and Silvan Sievers.
Fast Downward Stone Soup 2023.
In Tenth International Planning Competition (IPC 2023), Deterministic Part. 2023.
"""

OPTIMAL = False

CONFIGS = [
    # fdss-2018-01
    (383, ['--search', 'let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),let(hff, ff(transform=adapt_costs(one)),lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([hff,g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=false,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-03
    (57, ['--search', 'let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),let(hff, ff(transform=adapt_costs(one)),lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=true,bound=BOUND, verbosity=silent)))']),
    # lazy_hff_hlm-epsilon-greedy_pref_ops-no-reasonable-orders
    (60, ['--search', 'let(hlm, landmark_sum(lm_rhw(), pref=true, transform=adapt_costs(one)),let(hff, ff(transform=adapt_costs(one)),lazy(alt([single(hff),single(hff,pref_only=true),epsilon_greedy(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true, bound=BOUND, verbosity=silent)))']),
    # fdss-2014-01
    (22, ['--search', 'let(hadd, add(transform=adapt_costs(one)),let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone)),lazy_greedy([hadd,hlm],preferred=[hadd,hlm],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-04
    (30, ['--search', 'let(hff, ff(transform=adapt_costs(one)),let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),eager_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2014-08
    (206, ['--search', 'let(hff, ff(transform=adapt_costs(one)),let(hadd, add(transform=adapt_costs(one)),lazy_greedy([hadd,hff],preferred=[hadd,hff],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2014-03
    (30, ['--search', 'let(hadd, add(transform=adapt_costs(one)),let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone)),eager_greedy([hadd,hlm],preferred=[hadd,hlm],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-07
    (21, ['--search', 'let(hcea, cea(transform=adapt_costs(one)),let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),lazy_greedy([hcea,hlm],preferred=[hcea,hlm],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-09
    (59, ['--search', 'let(hff, ff(transform=adapt_costs(one)),lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true)],boost=2000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND, verbosity=silent))']),
    # fdss-2014-11
    (89, ['--search', 'let(hff, ff(transform=adapt_costs(one)),let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone)),lazy_wastar([hff,hlm],w=3,preferred=[hff,hlm],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-14
    (29, ['--search', 'let(hgoalcount, goalcount(transform=adapt_costs(plusone)),let(hff, ff(),lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hgoalcount,10)])),single(sum([g(),weight(hgoalcount,10)]),pref_only=true)],boost=2000),preferred=[hff,hgoalcount],reopen_closed=false,cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-1-03
    (53, ['--search', 'let(hcea, cea(transform=adapt_costs(one)),let(hcg, cg(transform=adapt_costs(one)),lazy_greedy([hcea,hcg],preferred=[hcea,hcg],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2014-16
    (19, ['--search', 'let(hcg, cg(transform=adapt_costs(one)),let(hff, ff(transform=adapt_costs(one)),lazy_wastar([hcg,hff],w=3,preferred=[hcg,hff],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-18
    (177, ['--search', 'let(hcg, cg(transform=adapt_costs(plusone)),lazy(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=true,cost_type=plusone,bound=BOUND, verbosity=silent))']),
    # fdss-2014-13
    (30, ['--search', 'let(hcg, cg(transform=adapt_costs(one)),let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone)),eager_greedy([hcg,hlm],preferred=[hcg,hlm],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2014-18
    (30, ['--search', 'let(hadd, add(transform=adapt_costs(one)),eager(alt([single(sum([g(), weight(hadd, 3)])),single(sum([g(), weight(hadd,3)]),pref_only=true)]),preferred=[hadd],cost_type=one,bound=BOUND, verbosity=silent))']),
    # fdss-1-11
    (26, ['--search', 'let(h, cea(transform=adapt_costs(one)),eager_greedy([h],preferred=[h],cost_type=one,bound=BOUND, verbosity=silent))']),
    # fdss-2014-19
    (27, ['--search', 'let(hff, ff(transform=adapt_costs(one)),let(hcea, cea(transform=adapt_costs(one)),eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hcea,3)])),single(sum([g(),weight(hcea,3)]),pref_only=true)]),preferred=[hff,hcea],cost_type=one,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-02
    (18, ['--search', 'let(lmg, lm_rhw(disjunctive_landmarks=true,use_orders=false),let(hlm, landmark_cost_partitioning(lmg,transform=adapt_costs(one)),let(hff, ff(transform=adapt_costs(one)),lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=0),preferred=[hlm],reopen_closed=false,cost_type=plusone,bound=BOUND, verbosity=silent))))']),
    # fdss-2018-16
    (29, ['--search', 'let(lmg, lm_rhw(disjunctive_landmarks=false,use_orders=true),let(hlm, landmark_sum(lmg,transform=adapt_costs(one)),let(hff, ff(transform=adapt_costs(one)),let(hblind, blind(),lazy(alt([type_based([g()]),single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hblind,2)]),pref_only=true),single(sum([g(),weight(hlm,2)])),single(sum([g(),weight(hlm,2)]),pref_only=true),single(sum([g(),weight(hff,2)])),single(sum([g(),weight(hff,2)]),pref_only=true)],boost=4419),preferred=[hlm],reopen_closed=true,cost_type=one,bound=BOUND, verbosity=silent)))))']),
    # fdss-2018-29
    (90, ['--search', 'let(hadd, add(transform=adapt_costs(plusone)),let(hff, ff(),lazy(alt([tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff]),tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd]),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd],pref_only=true)],boost=2537),preferred=[hff,hadd],reopen_closed=true,bound=BOUND, verbosity=silent)))']),
    # fdss-2018-31
    (28, ['--search', 'let(hff, ff(transform=adapt_costs(one)),lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND, verbosity=silent))']),
    # fdss-2018-28
    (29, ['--search', 'let(hblind, blind(),let(hadd, add(),let(hcg, cg(transform=adapt_costs(one)),let(hhmax, hmax(),eager(alt([tiebreaking([sum([g(),weight(hblind,7)]),hblind]),tiebreaking([sum([g(),weight(hhmax,7)]),hhmax]),tiebreaking([sum([g(),weight(hadd,7)]),hadd]),tiebreaking([sum([g(),weight(hcg,7)]),hcg])],boost=2142),preferred=[],reopen_closed=true,bound=BOUND, verbosity=silent)))))']),
    # fdss-2018-35
    (85, ['--search', 'let(lmg, lm_hm(conjunctive_landmarks=false,use_orders=false,m=1),let(hcg, cg(transform=adapt_costs(one)),let(hlm, landmark_cost_partitioning(lmg),lazy(alt([single(hlm),single(hlm,pref_only=true),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND, verbosity=silent))))']),
    # fdss-2018-27
    (30, ['--search', 'let(lmg, lm_reasonable_orders_hps(lm_rhw(disjunctive_landmarks=true,use_orders=true)),let(hblind, blind(),let(hadd, add(),let(hlm, landmark_sum(lmg,pref=true,transform=adapt_costs(plusone)),let(hff, ff(),lazy(alt([single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hlm,3)])),single(sum([weight(g(),2),weight(hlm,3)]),pref_only=true),single(sum([weight(g(),2),weight(hadd,3)])),single(sum([weight(g(),2),weight(hadd,3)]),pref_only=true)],boost=2474),preferred=[hadd],reopen_closed=false,cost_type=one,bound=BOUND, verbosity=silent))))))']),
    # fdss-2018-39
    (59, ['--search', 'let(lmg, lm_exhaust(),let(hgoalcount, goalcount(transform=adapt_costs(plusone)),let(hlm, landmark_sum(lmg),let(hff, ff(),let(hblind, blind(),eager(alt([tiebreaking([sum([weight(g(),8),weight(hblind,9)]),hblind]),tiebreaking([sum([weight(g(),8),weight(hlm,9)]),hlm]),tiebreaking([sum([weight(g(),8),weight(hff,9)]),hff]),tiebreaking([sum([weight(g(),8),weight(hgoalcount,9)]),hgoalcount])],boost=2005),preferred=[],reopen_closed=true,bound=BOUND, verbosity=silent))))))']),
]
