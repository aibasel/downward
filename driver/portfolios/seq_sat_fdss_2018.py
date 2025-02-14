"""
This is the "Fast Downward Stone Soup 2018" sequential portfolio that participated in the IPC 2018
satisficing and bounded-cost tracks. For more information, see the planner abstract:

Jendrik Seipp and Gabriele Röger.
Fast Downward Stone Soup 2018.
In Ninth International Planning Competition (IPC 2018), Deterministic Part, pp. 80-82. 2018.
https://ai.dmi.unibas.ch/papers/seipp-roeger-ipc2018.pdf
"""

OPTIMAL = False
CONFIGS = [
    (26, ["--search",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),type_based([hff,g()])],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=false,bound=BOUND)))"]),
    (25, ["--search",
        "let(lmg, lm_rhw(disjunctive_landmarks=true,use_orders=false),"
        "let(hlm, landmark_cost_partitioning(lmg,transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=0),preferred=[hlm],reopen_closed=false,cost_type=plusone,bound=BOUND))))"]),
    (135, ["--search",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=false,preferred_successors_first=true,bound=BOUND)))"]),
    (59, ["--search",
        "let(hff, ff(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "eager_greedy([hff,hlm],preferred=[hff,hlm],cost_type=one,bound=BOUND)))"]),
    (23, ["--search",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true)],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false,randomize_successors=true,preferred_successors_first=true,bound=BOUND)))"]),
    (57, ["--search",
        "let(lmg, lm_rhw(disjunctive_landmarks=true,use_orders=false),"
        "let(hcg, cg(transform=adapt_costs(plusone)),"
        "let(hlm, landmark_cost_partitioning(lmg,transform=adapt_costs(plusone)),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "lazy(alt([single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=1000),preferred=[hlm,hcg],reopen_closed=false,cost_type=plusone,bound=BOUND)))))"]),
    (17, ["--search",
        "let(hcea, cea(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "lazy_greedy([hcea,hlm],preferred=[hcea,hlm],cost_type=one,bound=BOUND)))"]),
    (12, ["--search",
        "let(hadd, add(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "lazy(alt([type_based([g()]),single(hadd),single(hadd,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hadd,hlm],cost_type=one,bound=BOUND)))"]),
    (26, ["--search",
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true)],boost=2000),preferred=[hff],reopen_closed=false,cost_type=one,bound=BOUND))"]),
    (28, ["--search",
        "let(hcg, cg(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "eager(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true),single(hlm),single(hlm,pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)))"]),
    (29, ["--search",
        "let(lmg, lm_rhw(disjunctive_landmarks=true,use_orders=true),"
        "let(hcea, cea(transform=adapt_costs(plusone)),"
        "let(hlm, landmark_cost_partitioning(lmg,transform=adapt_costs(plusone)),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true),single(hcea),single(hcea,pref_only=true)],boost=0),preferred=[hlm,hcea],reopen_closed=false,cost_type=plusone,bound=BOUND)))))"]),
    (88, ["--search",
        "let(hcea, cea(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "lazy_wastar([hcea,hlm],w=3,preferred=[hcea,hlm],cost_type=one,bound=BOUND)))"]),
    (8, ["--search",
        "let(hcg, cg(transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=100),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND)))"]),
    (54, ["--search",
        "let(hgoalcount, goalcount(transform=adapt_costs(plusone)),"
        "let(hff, ff(),"
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hgoalcount,10)])),single(sum([g(),weight(hgoalcount,10)]),pref_only=true)],boost=2000),preferred=[hff,hgoalcount],reopen_closed=false,cost_type=one,bound=BOUND)))"]),
    (24, ["--search",
        "let(hff, ff(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "eager(alt([type_based([g()]),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hff,hlm],cost_type=one,bound=BOUND)))"]),
    (29, ["--search",
        "let(lmg, lm_rhw(disjunctive_landmarks=false,use_orders=true),"
        "let(hlm, landmark_sum(lmg,transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "let(hblind, blind(),"
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hblind,2)]),pref_only=true),single(sum([g(),weight(hlm,2)])),single(sum([g(),weight(hlm,2)]),pref_only=true),single(sum([g(),weight(hff,2)])),single(sum([g(),weight(hff,2)]),pref_only=true)],boost=4419),preferred=[hlm],reopen_closed=true,cost_type=one,bound=BOUND)))))"]),
    (30, ["--search",
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy_wastar([hff],w=3,preferred=[hff],cost_type=one,bound=BOUND))"]),
    (28, ["--search",
        "let(hcg, cg(transform=adapt_costs(plusone)),"
        "lazy(alt([type_based([g()]),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=true,cost_type=plusone,bound=BOUND))"]),
    (58, ["--search",
        "let(hcg, cg(transform=adapt_costs(one)),"
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one)),"
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true)]),preferred=[hcg,hlm],cost_type=one,bound=BOUND)))"]),
    (26, ["--search",
        "let(hcea, cea(transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "let(hblind, blind(),"
        "eager(alt([single(sum([g(),weight(hblind,10)])),single(sum([g(),weight(hblind,10)]),pref_only=true),single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hcea,10)])),single(sum([g(),weight(hcea,10)]),pref_only=true)],boost=536),preferred=[hff],reopen_closed=false,bound=BOUND))))"]),
    (27, ["--search",
        "let(hcea, cea(transform=adapt_costs(one)),"
        "eager_greedy([hcea],preferred=[hcea],cost_type=one,bound=BOUND))"]),
    (50, ["--search",
        "let(hff, ff(transform=adapt_costs(one)),"
        "eager(alt([single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true)]),preferred=[hff],cost_type=one,bound=BOUND))"]),
    (28, ["--search",
        "let(hgoalcount, goalcount(transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "let(hblind, blind(),"
        "let(hcg, cg(),"
        "lazy(alt([type_based([g()]),single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hcg,3)])),single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hgoalcount,3)])),single(sum([weight(g(),2),weight(hgoalcount,3)]),pref_only=true)],boost=3662),preferred=[hff],reopen_closed=true,bound=BOUND)))))"]),
    (29, ["--search",
        "let(hgoalcount, goalcount(transform=adapt_costs(one)),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "let(hblind, blind(),"
        "let(hcg, cg(),"
        "lazy(alt([single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hcg,3)])),single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),single(sum([weight(g(),2),weight(hgoalcount,3)])),single(sum([weight(g(),2),weight(hgoalcount,3)]),pref_only=true)],boost=3662),preferred=[hff],reopen_closed=true,bound=BOUND)))))"]),
    (21, ["--search",
        "let(hcg, cg(transform=adapt_costs(plusone)),"
        "lazy(alt([single(sum([g(),weight(hcg,10)])),single(sum([g(),weight(hcg,10)]),pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=plusone,bound=BOUND))"]),
    (21, ["--search",
        "let(hcg, cg(transform=adapt_costs(one)),"
        "eager(alt([single(sum([g(),weight(hcg,3)])),single(sum([g(),weight(hcg,3)]),pref_only=true)]),preferred=[hcg],cost_type=one,bound=BOUND))"]),
    (24, ["--search",
        "let(lmg, lm_reasonable_orders_hps(lm_rhw(disjunctive_landmarks=true,use_orders=true)),"
        "let(hblind, blind(),"
        "let(hadd, add(),"
        "let(hlm, landmark_sum(lmg,pref=true,transform=adapt_costs(plusone)),"
        "let(hff, ff(),"
        "lazy(alt([single(sum([weight(g(),2),weight(hblind,3)])),single(sum([weight(g(),2),weight(hblind,3)]),pref_only=true),single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),single(sum([weight(g(),2),weight(hlm,3)])),single(sum([weight(g(),2),weight(hlm,3)]),pref_only=true),single(sum([weight(g(),2),weight(hadd,3)])),single(sum([weight(g(),2),weight(hadd,3)]),pref_only=true)],boost=2474),preferred=[hadd],reopen_closed=false,cost_type=one,bound=BOUND))))))"]),
    (28, ["--search",
        "let(hblind, blind(),"
        "let(hadd, add(),"
        "let(hcg, cg(transform=adapt_costs(one)),"
        "let(hhmax, hmax(),"
        "eager(alt([tiebreaking([sum([g(),weight(hblind,7)]),hblind]),tiebreaking([sum([g(),weight(hhmax,7)]),hhmax]),tiebreaking([sum([g(),weight(hadd,7)]),hadd]),tiebreaking([sum([g(),weight(hcg,7)]),hcg])],boost=2142),preferred=[],reopen_closed=true,bound=BOUND)))))"]),
    (28, ["--search",
        "let(hadd, add(transform=adapt_costs(plusone)),"
        "let(hff, ff(),"
        "lazy(alt([tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff]),tiebreaking([sum([weight(g(),4),weight(hff,5)]),hff],pref_only=true),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd]),tiebreaking([sum([weight(g(),4),weight(hadd,5)]),hadd],pref_only=true)],boost=2537),preferred=[hff,hadd],reopen_closed=true,bound=BOUND)))"]),
    (53, ["--search",
        "let(lmg, lm_hm(conjunctive_landmarks=false,use_orders=false,m=1),"
        "let(hlm, landmark_cost_partitioning(lmg,transform=adapt_costs(plusone)),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=5000),preferred=[hlm],reopen_closed=false,bound=BOUND))))"]),
    (29, ["--search",
        "let(hff, ff(transform=adapt_costs(one)),"
        "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),single(sum([weight(g(),2),weight(hff,3)]),pref_only=true)],boost=5000),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND))"]),
    (27, ["--search",
        "let(hblind, blind(),"
        "let(hff, ff(transform=adapt_costs(one)),"
        "eager(alt([single(sum([g(),weight(hblind,2)])),single(sum([g(),weight(hff,2)]))],boost=4480),preferred=[],reopen_closed=true,bound=BOUND)))"]),
    (29, ["--search",
        "let(lmg, lm_hm(conjunctive_landmarks=false,use_orders=false,m=1),"
        "let(hlm, landmark_cost_partitioning(lmg),"
        "let(hff, ff(),"
        "lazy(alt([type_based([g()]),single(hlm),single(hlm,pref_only=true),single(hff),single(hff,pref_only=true)],boost=1000),preferred=[hlm,hff],reopen_closed=false,cost_type=one,bound=BOUND))))"]),
    (54, ["--search",
        "let(lmg, lm_hm(conjunctive_landmarks=true,use_orders=true,m=1),"
        "let(hlm, landmark_cost_partitioning(lmg),"
        "let(hff, ff(),"
        "lazy(alt([tiebreaking([sum([g(),weight(hlm,10)]),hlm]),tiebreaking([sum([g(),weight(hlm,10)]),hlm],pref_only=true),tiebreaking([sum([g(),weight(hff,10)]),hff]),tiebreaking([sum([g(),weight(hff,10)]),hff],pref_only=true)],boost=200),preferred=[hlm],reopen_closed=true,cost_type=plusone,bound=BOUND))))"]),
    (87, ["--search",
        "let(lmg, lm_hm(conjunctive_landmarks=false,use_orders=false,m=1),"
        "let(hcg, cg(transform=adapt_costs(one)),"
        "let(hlm, landmark_cost_partitioning(lmg),"
        "lazy(alt([single(hlm),single(hlm,pref_only=true),single(hcg),single(hcg,pref_only=true)],boost=0),preferred=[hcg],reopen_closed=false,cost_type=one,bound=BOUND))))"]),
    (30, ["--search",
        "let(lmg, lm_exhaust(),"
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "let(hhmax, hmax(),"
        "let(hblind, blind(),"
        "let(hlm, landmark_cost_partitioning(lmg,pref=false,transform=adapt_costs(one)),"
        "lazy(alt([type_based([g()]),single(sum([g(),weight(hblind,3)])),single(sum([g(),weight(hblind,3)]),pref_only=true),single(sum([g(),weight(hff,3)])),single(sum([g(),weight(hff,3)]),pref_only=true),single(sum([g(),weight(hlm,3)])),single(sum([g(),weight(hlm,3)]),pref_only=true),single(sum([g(),weight(hhmax,3)])),single(sum([g(),weight(hhmax,3)]),pref_only=true)],boost=3052),preferred=[hff],reopen_closed=true,bound=BOUND))))))"]),
    (56, ["--search",
        "let(hff, ff(transform=adapt_costs(plusone)),"
        "lazy(alt([tiebreaking([sum([g(),hff]),hff]),tiebreaking([sum([g(),hff]),hff],pref_only=true)],boost=432),preferred=[hff],reopen_closed=true,cost_type=one,bound=BOUND))"]),
    (19, ["--search",
        "let(lmg, lm_merged([lm_rhw(disjunctive_landmarks=false,use_orders=true),lm_hm(m=1,conjunctive_landmarks=true,use_orders=true)]),"
        "let(hff, ff(),"
        "let(hlm, landmark_cost_partitioning(lmg),"
        "lazy(alt([single(sum([g(),weight(hff,10)])),single(sum([g(),weight(hff,10)]),pref_only=true),single(sum([g(),weight(hlm,10)])),single(sum([g(),weight(hlm,10)]),pref_only=true)],boost=500),preferred=[hff],reopen_closed=false,cost_type=plusone,bound=BOUND))))"]),
    (56, ["--search",
        "let(lmg, lm_exhaust(),"
        "let(hgoalcount, goalcount(transform=adapt_costs(plusone)),"
        "let(hlm, landmark_sum(lmg),"
        "let(hff, ff(),"
        "let(hblind, blind(),"
        "eager(alt([tiebreaking([sum([weight(g(),8),weight(hblind,9)]),hblind]),tiebreaking([sum([weight(g(),8),weight(hlm,9)]),hlm]),tiebreaking([sum([weight(g(),8),weight(hff,9)]),hff]),tiebreaking([sum([weight(g(),8),weight(hgoalcount,9)]),hgoalcount])],boost=2005),preferred=[],reopen_closed=true,bound=BOUND))))))"]),
    (24, ["--search",
        "let(lmg, lm_zg(use_orders=false),"
        "let(hlm, landmark_cost_partitioning(lmg,pref=false),"
        "eager(single(sum([g(),weight(hlm,3)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)))"]),
    (81, ["--search",
        "let(lmg, lm_hm(conjunctive_landmarks=true,use_orders=false,m=1),"
        "let(hlm, landmark_cost_partitioning(lmg),"
        "eager(single(sum([g(),weight(hlm,5)])),preferred=[],reopen_closed=true,cost_type=one,bound=BOUND)))"]),
]
