def configs_optimal_core():
    return {
        # A*
        "astar_blind": [
            "--search",
            "astar(blind)"],
        "astar_h2": [
            "--search",
            "astar(hm(2))"],
        "astar_ipdb": [
            "--search",
            "astar(ipdb)"],
        "bjolp": [
            "--evaluator",
            "lmc=lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true)",
            "--search",
            "astar(lmc,lazy_evaluator=lmc)"],
        "astar_lmcut": [
            "--search",
            "astar(lmcut)"],
        "astar_hmax": [
            "--search",
            "astar(hmax)"],
        "astar_merge_and_shrink_rl_fh": [
            "--search",
            "astar(merge_and_shrink("
            "merge_strategy=merge_strategy=merge_precomputed("
            "merge_tree=linear(variable_order=reverse_level)),"
            "shrink_strategy=shrink_fh(),"
            "label_reduction=exact(before_shrinking=false,"
            "before_merging=true),max_states=50000,verbosity=silent))"],
        "astar_merge_and_shrink_dfp_bisim": [
            "--search",
            "astar(merge_and_shrink(merge_strategy=merge_stateless("
            "merge_selector=score_based_filtering(scoring_functions=["
            "goal_relevance,dfp,total_order("
            "atomic_ts_order=reverse_level,product_ts_order=new_to_old,"
            "atomic_before_product=false)])),"
            "shrink_strategy=shrink_bisimulation(greedy=false),"
            "label_reduction=exact(before_shrinking=true,"
            "before_merging=false),max_states=50000,"
            "threshold_before_merge=1,verbosity=silent))"],
        "astar_merge_and_shrink_dfp_greedy_bisim": [
            "--search",
            "astar(merge_and_shrink(merge_strategy=merge_stateless("
            "merge_selector=score_based_filtering(scoring_functions=["
            "goal_relevance,dfp,total_order("
            "atomic_ts_order=reverse_level,product_ts_order=new_to_old,"
            "atomic_before_product=false)])),"
            "shrink_strategy=shrink_bisimulation("
            "greedy=true),"
            "label_reduction=exact(before_shrinking=true,"
            "before_merging=false),max_states=infinity,"
            "threshold_before_merge=1,verbosity=silent))"],
        "blind-sss-simple": [
            "--search",
            "astar(blind(), pruning=stubborn_sets_simple())"],
        "blind-sss-ec": [
            "--search", "astar(blind(), pruning=stubborn_sets_ec())"],
        "blind-atom-centric-sss": [
            "--search", "astar(blind(), pruning=atom_centric_stubborn_sets())"],
    }


def configs_satisficing_core():
    return {
        # A*
        "astar_goalcount": [
            "--search",
            "astar(goalcount)"],
        # eager greedy
        "eager_greedy_ff": [
            "--evaluator",
            "h=ff()",
            "--search",
            "eager_greedy([h],preferred=[h])"],
        "eager_greedy_add": [
            "--evaluator",
            "h=add()",
            "--search",
            "eager_greedy([h],preferred=[h])"],
        "eager_greedy_cg": [
            "--evaluator",
            "h=cg()",
            "--search",
            "eager_greedy([h],preferred=[h])"],
        "eager_greedy_cea": [
            "--evaluator",
            "h=cea()",
            "--search",
            "eager_greedy([h],preferred=[h])"],
        # lazy greedy
        "lazy_greedy_ff": [
            "--evaluator",
            "h=ff()",
            "--search",
            "lazy_greedy([h],preferred=[h])"],
        "lazy_greedy_add": [
            "--evaluator",
            "h=add()",
            "--search",
            "lazy_greedy([h],preferred=[h])"],
        "lazy_greedy_cg": [
            "--evaluator",
            "h=cg()",
            "--search",
            "lazy_greedy([h],preferred=[h])"],
        # LAMA first
        "lama-first": [
            "--evaluator",
            "hlm=lmcount(lm_factory=lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=false)",
            "--evaluator", "hff=ff(transform=adapt_costs(one))",
            "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                                       cost_type=one,reopen_closed=false)"""],
        "lama-first-typed": [
            "--evaluator",
            "hlm=lmcount(lm_factory=lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=false)",
            "--evaluator", "hff=ff(transform=adapt_costs(one))",
            "--search",
                "lazy(alt([single(hff), single(hff, pref_only=true),"
                "single(hlm), single(hlm, pref_only=true), type_based([hff, g()])], boost=1000),"
                "preferred=[hff,hlm], cost_type=one, reopen_closed=false, randomize_successors=true,"
                "preferred_successors_first=false)"],
    }


def configs_optimal_extended():
    return {
        "astar_cegar": [
            "--search",
            "astar(cegar())"],
        "pdb": [
            "--search",
            "astar(pdb())"],
    }


def configs_satisficing_extended():
    return {
        # eager greedy
        "eager_greedy_alt_ff_cg": [
            "--evaluator",
            "hff=ff()",
            "--evaluator",
            "hcg=cg()",
            "--search",
            "eager_greedy([hff,hcg],preferred=[hff,hcg])"],
        "eager_greedy_ff_no_pref": [
            "--search",
            "eager_greedy([ff()])"],
        # lazy greedy
        "lazy_greedy_alt_cea_cg": [
            "--evaluator",
            "hcea=cea()",
            "--evaluator",
            "hcg=cg()",
            "--search",
            "lazy_greedy([hcea,hcg],preferred=[hcea,hcg])"],
        "lazy_greedy_ff_no_pref": [
            "--search",
            "lazy_greedy([ff()])"],
        "lazy_greedy_cea": [
            "--evaluator",
            "h=cea()",
            "--search",
            "lazy_greedy([h],preferred=[h])"],
        # lazy wA*
        "lazy_wa3_ff": [
            "--evaluator",
            "h=ff()",
            "--search",
            "lazy_wastar([h],w=3,preferred=[h])"],
        # eager wA*
        "eager_wa3_cg": [
            "--evaluator",
            "h=cg()",
            "--search",
            "eager(single(sum([g(),weight(h,3)])),preferred=[h])"],
        # ehc
        "ehc_ff": [
            "--search",
            "ehc(ff())"],
        # iterated
        "iterated_wa_ff": [
            "--evaluator",
            "h=ff()",
            "--search",
            "iterated([lazy_wastar([h],w=10), lazy_wastar([h],w=5), lazy_wastar([h],w=3),"
            "lazy_wastar([h],w=2), lazy_wastar([h],w=1)])"],
        # pareto open list
        "pareto_ff": [
            "--evaluator",
            "h=ff()",
            "--search",
            "eager(pareto([sum([g(), h]), h]), reopen_closed=true,"
            "f_eval=sum([g(), h]))"],
    }


def configs_optimal_lp(lp_solver="CPLEX"):
    return {
        "divpot": ["--search", f"astar(diverse_potentials(lpsolver={lp_solver}))"],
        "seq+lmcut": ["--search", f"astar(operatorcounting([state_equation_constraints(), lmcut_constraints()], lpsolver={lp_solver}))"],
    }


def default_configs_optimal(core=True, extended=True):
    configs = {}
    if core:
        configs.update(configs_optimal_core())
    if extended:
        configs.update(configs_optimal_extended())
    return configs


def default_configs_satisficing(core=True, extended=True):
    configs = {}
    if core:
        configs.update(configs_satisficing_core())
    if extended:
        configs.update(configs_satisficing_extended())
    return configs
