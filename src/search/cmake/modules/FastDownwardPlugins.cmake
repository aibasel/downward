## == Details of the core source files ==
##
## If you're adding another .cc file to the codebase which *isn't* a plugin, add
## it to the following list

set(search_main_SRCS
        abstract_task.cc
        axioms.cc
        causal_graph.cc
        combining_evaluator.cc
        cost_adapted_task.cc
        countdown_timer.cc
        delegating_task.cc
        doc.cc
        domain_transition_graph.cc
        eager_search.cc
        enforced_hill_climbing_search.cc
        equivalence_relation.cc
        evaluator.cc
        exact_timer.cc
        g_evaluator.cc
        global_operator.cc
        globals.cc
        global_state.cc
        heuristic.cc
        int_packer.cc
        ipc_max_heuristic.cc
        iterated_search.cc
        lazy_search.cc
        lp_internals.cc
        lp_solver.cc
        max_evaluator.cc
        operator_cost.cc
        option_parser.cc
        option_parser_util.cc
        per_state_information.cc
        plugin.cc
        pref_evaluator.cc
        priority_queue.cc
        relaxation_heuristic.cc
        rng.cc
        root_task.cc
        scalar_evaluator.cc
        search_engine.cc
        search_node_info.cc
        search_progress.cc
        search_space.cc
        segmented_vector.cc
        state_id.cc
        state_registry.cc
        successor_generator.cc
        sum_evaluator.cc
        task_proxy.cc
        task_tools.cc
        timer.cc
        tracer.cc
        utilities.cc
        utilities_hash.cc
        variable_order_finder.cc
        weighted_evaluator.cc
        open_lists/alternation_open_list.cc
        open_lists/open_list_buckets.cc
        open_lists/open_list.cc
        open_lists/pareto_open_list.cc
        open_lists/standard_scalar_open_list.cc
        open_lists/tiebreaking_open_list.cc
)


## Details of the plugins
#
# For now, everything defaults to being enabled - it's up to the user to specify -DPLUGIN_FOO=FALSE to disable
# a given plugin

# First, initialise empty list of plugin source files

set(search_plugin_SRCS

)


# The recipe for defining a pluging should be fairly self-explanatory.  The text in double-quotes is
# a free-form natural language description, used in the CMake GUI to explain what the flag means.

option(PLUGIN_ADDITIVE_HEURISTIC "The additive heuristic" TRUE)
if(PLUGIN_ADDITIVE_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} additive_heuristic.cc)
endif()


option(PLUGIN_BLIND_SEARCH_HEURISTIC "The 'blind search' heuristic" TRUE)
if(PLUGIN_BLIND_SEARCH_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} blind_search_heuristic.cc)
endif()


option(PLUGIN_CEA_HEURISTIC "The context-enhanced additive heuristic" TRUE)
if(PLUGIN_CEA_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} cea_heuristic.cc)
endif()


option(PLUGIN_CG_HEURISTIC "The causal graph heuristic" TRUE)
if(PLUGIN_CG_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} cg_heuristic.cc cg_cache.cc)
endif()


option(PLUGIN_FF_HEURISTIC "The FF heuristic (an implementation of the RPG heuristic)" TRUE)
if(PLUGIN_FF_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} ff_heuristic.cc)
endif()


option(PLUGIN_GOAL_COUNT_HEURISTIC "The goal-counting heuristic" TRUE)
if(PLUGIN_GOAL_COUNT_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} goal_count_heuristic.cc)
endif()


option(PLUGIN_HM_HEURISTIC "The h^m heuristic" TRUE)
if(PLUGIN_HM_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} hm_heuristic.cc)
endif()


option(PLUGIN_LM_CUT_HEURISTIC "The LM-cut heuristic" TRUE)
if(PLUGIN_LM_CUT_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} lm_cut_heuristic.cc)
endif()


option(PLUGIN_MAX_HEURISTIC "The Max heuristic" TRUE)
if(PLUGIN_MAX_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} max_heuristic.cc)
endif()


option(PLUGIN_MAS_HEURISTIC "The Merge-and-Shrink heuristic" TRUE)
if(PLUGIN_MAS_HEURISTIC)
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        merge_and_shrink/labels.cc
        merge_and_shrink/merge_and_shrink_heuristic.cc
        merge_and_shrink/merge_dfp.cc
        merge_and_shrink/merge_linear.cc
        merge_and_shrink/merge_strategy.cc
        merge_and_shrink/shrink_bisimulation.cc
        merge_and_shrink/shrink_bucket_based.cc
        merge_and_shrink/shrink_fh.cc
        merge_and_shrink/shrink_random.cc
        merge_and_shrink/shrink_strategy.cc
        merge_and_shrink/transition_system.cc
    )
endif()


option(PLUGIN_LANDMARKS "Plugin containing the code to reason with landmarks" TRUE)
if(PLUGIN_LANDMARKS)
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        landmarks/exploration.cc
        landmarks/h_m_landmarks.cc
        landmarks/lama_ff_synergy.cc
        landmarks/landmark_cost_assignment.cc
        landmarks/landmark_count_heuristic.cc
        landmarks/landmark_factory.cc
        landmarks/landmark_factory_rpg_exhaust.cc
        landmarks/landmark_factory_rpg_sasp.cc
        landmarks/landmark_factory_zhu_givan.cc
        landmarks/landmark_graph.cc
        landmarks/landmark_graph_merged.cc
        landmarks/landmark_status_manager.cc
        landmarks/landmark_types.cc
        landmarks/util.cc
    )
endif()


option(PLUGIN_LEARNING "Plugin containing the code to reason with learning" FALSE)
if(PLUGIN_LEARNING)
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        learning/AODE.cc
        learning/classifier.cc
        learning/composite_feature_extractor.cc
        learning/feature_extractor.cc
        learning/maximum_heuristic.cc
        learning/naive_bayes_classifier.cc
        learning/PDB_state_space_sample.cc
        learning/probe_state_space_sample.cc
        learning/selective_max_heuristic.cc
        learning/state_space_sample.cc
        learning/state_vars_feature_extractor.cc
    )
endif()


option(PLUGIN_PDBS "Plugin containing the code for PDBs" TRUE)
if(PLUGIN_PDBS)
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        pdbs/canonical_pdbs_heuristic.cc
        pdbs/dominance_pruner.cc
        pdbs/match_tree.cc
        pdbs/max_cliques.cc
        pdbs/pattern_generation_edelkamp.cc
        pdbs/pattern_generation_haslum.cc
        pdbs/pdb_heuristic.cc
        pdbs/util.cc
        pdbs/zero_one_pdbs_heuristic.cc
    )
endif()
