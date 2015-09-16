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
        "astar_lmcount_lm_merged_rhw_hm": [
            "--search",
            "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"],
        "astar_lmcut": [
            "--search",
            "astar(lmcut)"],
        "astar_hmax": [
            "--search",
            "astar(hmax)"],
        "astar_merge_and_shrink_rl_fh": [
            "--search",
            "astar(merge_and_shrink("
            "merge_strategy=merge_linear(variable_order=reverse_level),"
            "shrink_strategy=shrink_fh(max_states=50000),"
            "label_reduction=label_reduction(before_shrinking=false,"
            "before_merging=true)))"],
        "astar_merge_and_shrink_dfp_bisim": [
            "--search",
            "astar(merge_and_shrink(merge_strategy=merge_dfp,"
            "shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,"
            "greedy=false),"
            "label_reduction=label_reduction(before_shrinking=true,"
            "before_merging=false)))"],
        "astar_merge_and_shrink_dfp_greedy_bisim": [
            "--search",
            "astar(merge_and_shrink(merge_strategy=merge_dfp,"
            "shrink_strategy=shrink_bisimulation(max_states=infinity,threshold=1,"
            "greedy=true),"
            "label_reduction=label_reduction(before_shrinking=true,"
            "before_merging=false)))"],
        "astar_selmax_lmcut_lmcount": [
            "--search",
            "astar(selmax([lmcut(),lmcount(lm_merged([lm_hm(m=1),lm_rhw()]),"
            "admissible=true)],training_set=1000),mpd=true)"],
    }

MERGE_AND_SHRINK = ('astar(merge_and_shrink('
    'merge_strategy=merge_dfp,'
        'shrink_strategy=shrink_bisimulation('
         'max_states=50000,'
        'threshold=1,'
        'greedy=false),'
    'label_reduction=label_reduction('
        'before_shrinking=true,'
        'before_merging=false)'
'))')


def configs_satisficing_core():
    return {
        # A*
        "astar_goalcount": [
            "--search",
            "astar(goalcount)"],
        # eager greedy
        "eager_greedy_ff": [
            "--heuristic",
            "h=ff()",
            "--search",
            "eager_greedy(h, preferred=h)"],
        "eager_greedy_add": [
            "--heuristic",
            "h=add()",
            "--search",
            "eager_greedy(h, preferred=h)"],
        "eager_greedy_cg": [
            "--heuristic",
            "h=cg()",
            "--search",
            "eager_greedy(h, preferred=h)"],
        "eager_greedy_cea": [
            "--heuristic",
            "h=cea()",
            "--search",
            "eager_greedy(h, preferred=h)"],
        # lazy greedy
        "lazy_greedy_ff": [
            "--heuristic",
            "h=ff()",
            "--search",
            "lazy_greedy(h, preferred=h)"],
        "lazy_greedy_add": [
            "--heuristic",
            "h=add()",
            "--search",
            "lazy_greedy(h, preferred=h)"],
        "lazy_greedy_cg": [
            "--heuristic",
            "h=cg()",
            "--search",
            "lazy_greedy(h, preferred=h)"],
    }


def configs_optimal_ipc():
    return {
        "seq_opt_merge_and_shrink": ["--alias", "seq-opt-merge-and-shrink"],
        "seq_opt_fdss_1": ["--alias", "seq-opt-fdss-1"],
        "seq_opt_fdss_2": ["--alias", "seq-opt-fdss-2"],
    }


def configs_satisficing_ipc():
    return {
        "seq_sat_lama_2011": ["--alias", "seq-sat-lama-2011"],
        "seq_sat_fdss_1": ["--alias", "seq-sat-fdss-1"],
        "seq_sat_fdss_2": ["--alias", "seq-sat-fdss-2"],
    }


def configs_optimal_extended():
    return {
        # A*
        "astar_lmcount_lm_merged_rhw_hm_no_order": [
            "--search",
            "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)"],
    }


def configs_satisficing_extended():
    return {
        # eager greedy
        "eager_greedy_alt_ff_cg": [
            "--heuristic",
            "hff=ff()",
            "--heuristic",
            "hcg=cg()",
            "--search",
            "eager_greedy(hff,hcg,preferred=[hff,hcg])"],
        "eager_greedy_ff_no_pref": [
            "--search",
            "eager_greedy(ff())"],
        # lazy greedy
        "lazy_greedy_alt_cea_cg": [
            "--heuristic",
            "hcea=cea()",
            "--heuristic",
            "hcg=cg()",
            "--search",
            "lazy_greedy(hcea,hcg,preferred=[hcea,hcg])"],
        "lazy_greedy_ff_no_pref": [
            "--search",
            "lazy_greedy(ff())"],
        "lazy_greedy_cea": [
            "--heuristic",
            "h=cea()",
            "--search",
            "lazy_greedy(h, preferred=h)"],
        # lazy wA*
        "lazy_wa3_ff": [
            "--heuristic",
            "h=ff()",
            "--search",
            "lazy_wastar(h,w=3,preferred=h)"],
        # eager wA*
        "eager_wa3_cg": [
            "--heuristic",
            "h=cg()",
            "--search",
            "eager(single(sum([g(),weight(h,3)])),preferred=h)"],
        # ehc
        "ehc_ff": [
            "--search",
            "ehc(ff())"],
        # iterated
        "iterated_wa_ff": [
            "--heuristic",
            "h=ff()",
            "--search",
            "iterated([lazy_wastar(h,w=10), lazy_wastar(h,w=5), lazy_wastar(h,w=3),"
            "lazy_wastar(h,w=2), lazy_wastar(h,w=1)])"],
        # pareto open list
        "pareto_ff": [
            "--heuristic",
            "h=ff()",
            "--search",
            "eager(pareto([sum([g(), h]), h]), reopen_closed=true,"
            "f_eval=sum([g(), h]))"],
        # bucket-based open list
        "bucket_lmcut": [
            "--heuristic",
            "h=lmcut()",
            "--search",
            "eager(single_buckets(h), reopen_closed=true)"],
    }


def task_transformation_test_configs():
    return {
        "root_task": [
            "--search", "lazy_greedy(ff())"],
        "root_task_no_transform": [
            "--search", "lazy_greedy(ff(transform=no_transform))"],
        "adapt_costs": [
            "--search", "lazy_greedy(ff(transform=adapt_costs(cost_type=plusone)))"],
        "adapt_adapted_costs": [
            "--search",
            "lazy_greedy(ff(transform=adapt_costs(cost_type=plusone,"
                           "transform=adapt_costs(cost_type=plusone))))"],
    }

def regression_test_configs():
    return {
        "pdb": ["--search", "astar(pdb())"],
    }


def default_configs_optimal(core=True, ipc=True, extended=False):
    configs = {}
    if core:
        configs.update(configs_optimal_core())
    if ipc:
        configs.update(configs_optimal_ipc())
    if extended:
        configs.update(configs_optimal_extended())
    return configs


def default_configs_satisficing(core=True, ipc=True, extended=False):
    configs = {}
    if core:
        configs.update(configs_satisficing_core())
    if ipc:
        configs.update(configs_satisficing_ipc())
    if extended:
        configs.update(configs_satisficing_extended())
    return configs
