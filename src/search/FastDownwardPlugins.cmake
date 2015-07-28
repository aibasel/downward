## == Details of the core source files ==
##
## If you're adding another file to the codebase which *isn't* a plugin, add
## it to the following list

set(PLANNER_SOURCES
        planner.cc

        abstract_task.cc abstract_task.h
        axioms.cc axioms.h
        causal_graph.cc causal_graph.h
        combining_evaluator.cc combining_evaluator.h
        cost_adapted_task.cc cost_adapted_task.h
        countdown_timer.cc countdown_timer.h
        delegating_task.cc delegating_task.h
        doc.h
        domain_transition_graph.cc domain_transition_graph.h
        eager_search.cc eager_search.h
        enforced_hill_climbing_search.cc enforced_hill_climbing_search.h
        equivalence_relation.cc equivalence_relation.h
        evaluation_context.cc evaluation_context.h
        evaluation_result.cc evaluation_result.h
        exact_timer.cc exact_timer.h
        g_evaluator.cc g_evaluator.h
        global_operator.cc global_operator.h
        globals.cc globals.h
        global_state.cc global_state.h
        heuristic_cache.cc heuristic_cache.h
        heuristic.cc heuristic.h
        int_packer.cc int_packer.h
        ipc_max_heuristic.cc ipc_max_heuristic.h
        iterated_search.cc iterated_search.h
        lazy_search.cc lazy_search.h
        lp_internals.cc lp_internals.h
        lp_solver.cc lp_solver.h
        max_evaluator.cc max_evaluator.h
        operator_cost.cc operator_cost.h
        option_parser.cc option_parser.h
        option_parser_util.cc option_parser_util.h
        per_state_information.cc per_state_information.h
        plugin.h
        pref_evaluator.cc pref_evaluator.h
        priority_queue.h
        relaxation_heuristic.cc relaxation_heuristic.h
        rng.cc rng.h
        root_task.cc root_task.h
        scalar_evaluator.cc scalar_evaluator.h
        search_engine.cc search_engine.h
        search_node_info.cc search_node_info.h
        search_progress.cc search_progress.h
        search_space.cc search_space.h
        search_statistics.cc search_statistics.h
        segmented_vector.cc segmented_vector.h
        state_id.cc state_id.h
        state_registry.cc state_registry.h
        successor_generator.cc successor_generator.h
        sum_evaluator.cc sum_evaluator.h
        task_proxy.cc task_proxy.h
        task_tools.cc task_tools.h
        timer.cc timer.h
        tracer.cc tracer.h
        utilities.cc utilities.h
        utilities_hash.cc utilities_hash.h
        utilities_windows.h
        variable_order_finder.cc variable_order_finder.h
        weighted_evaluator.cc weighted_evaluator.h
        open_lists/alternation_open_list.cc open_lists/alternation_open_list.h
        open_lists/bucket_open_list.cc open_lists/bucket_open_list.h
        open_lists/open_list.cc open_lists/open_list.h
        open_lists/pareto_open_list.cc open_lists/pareto_open_list.h
        open_lists/standard_scalar_open_list.cc open_lists/standard_scalar_open_list.h
        open_lists/tiebreaking_open_list.cc open_lists/tiebreaking_open_list.h
)


## Details of the plugins
#
# For now, everything defaults to being enabled - it's up to the user to specify -DPLUGIN_FOO=FALSE to disable
# a given plugin.
# The recipe for defining a plugin should be fairly self-explanatory. The text in double-quotes is
# a free-form natural language description, used in the CMake GUI to explain what the flag means.

option(PLUGIN_ADDITIVE_HEURISTIC "The additive heuristic" TRUE)
if(PLUGIN_ADDITIVE_HEURISTIC)
    list(APPEND PLANNER_SOURCES additive_heuristic)
endif()


option(PLUGIN_BLIND_SEARCH_HEURISTIC "The 'blind search' heuristic" TRUE)
if(PLUGIN_BLIND_SEARCH_HEURISTIC)
    list(APPEND PLANNER_SOURCES blind_search_heuristic)
endif()


option(PLUGIN_CEA_HEURISTIC "The context-enhanced additive heuristic" TRUE)
if(PLUGIN_CEA_HEURISTIC)
    list(APPEND PLANNER_SOURCES cea_heuristic)
endif()


option(PLUGIN_CG_HEURISTIC "The causal graph heuristic" TRUE)
if(PLUGIN_CG_HEURISTIC)
    list(APPEND PLANNER_SOURCES cg_heuristic cg_cache)
endif()


option(PLUGIN_FF_HEURISTIC "The FF heuristic (an implementation of the RPG heuristic)" TRUE)
if(PLUGIN_FF_HEURISTIC)
    list(APPEND PLANNER_SOURCES ff_heuristic)
endif()


option(PLUGIN_GOAL_COUNT_HEURISTIC "The goal-counting heuristic" TRUE)
if(PLUGIN_GOAL_COUNT_HEURISTIC)
    list(APPEND PLANNER_SOURCES goal_count_heuristic)
endif()


option(PLUGIN_HM_HEURISTIC "The h^m heuristic" TRUE)
if(PLUGIN_HM_HEURISTIC)
    list(APPEND PLANNER_SOURCES hm_heuristic)
endif()


option(PLUGIN_LM_CUT_HEURISTIC "The LM-cut heuristic" TRUE)
if(PLUGIN_LM_CUT_HEURISTIC)
    list(APPEND PLANNER_SOURCES lm_cut_heuristic)
endif()


option(PLUGIN_MAX_HEURISTIC "The Max heuristic" TRUE)
if(PLUGIN_MAX_HEURISTIC)
    list(APPEND PLANNER_SOURCES max_heuristic)
endif()


option(PLUGIN_MAS_HEURISTIC "The Merge-and-Shrink heuristic" TRUE)
if(PLUGIN_MAS_HEURISTIC)
    list(APPEND PLANNER_SOURCES
        merge_and_shrink/distances.h
        merge_and_shrink/factored_transition_system.h
        merge_and_shrink/fts_factory.h
        merge_and_shrink/heuristic_representation.h
        merge_and_shrink/labels
        merge_and_shrink/merge_and_shrink_heuristic
        merge_and_shrink/merge_dfp
        merge_and_shrink/merge_linear
        merge_and_shrink/merge_strategy
        merge_and_shrink/shrink_bisimulation
        merge_and_shrink/shrink_bucket_based
        merge_and_shrink/shrink_fh
        merge_and_shrink/shrink_random
        merge_and_shrink/shrink_strategy
        merge_and_shrink/transition_system
    )
endif()


option(PLUGIN_LANDMARKS "Plugin containing the code to reason with landmarks" TRUE)
if(PLUGIN_LANDMARKS)
    list(APPEND PLANNER_SOURCES
        landmarks/exploration
        landmarks/h_m_landmarks
        landmarks/lama_ff_synergy
        landmarks/landmark_cost_assignment
        landmarks/landmark_count_heuristic
        landmarks/landmark_factory
        landmarks/landmark_factory_rpg_exhaust
        landmarks/landmark_factory_rpg_sasp
        landmarks/landmark_factory_zhu_givan
        landmarks/landmark_graph
        landmarks/landmark_graph_merged
        landmarks/landmark_status_manager
        landmarks/landmark_types
        landmarks/util
    )
endif()


option(PLUGIN_LEARNING "Plugin containing the code to reason with learning" FALSE)
if(PLUGIN_LEARNING)
    list(APPEND PLANNER_SOURCES
        learning/AODE
        learning/classifier
        learning/composite_feature_extractor
        learning/feature_extractor
        learning/maximum_heuristic
        learning/naive_bayes_classifier
        learning/PDB_state_space_sample
        learning/probe_state_space_sample
        learning/selective_max_heuristic
        learning/state_space_sample
        learning/state_vars_feature_extractor
    )
endif()


option(PLUGIN_PDBS "Plugin containing the code for PDBs" TRUE)
if(PLUGIN_PDBS)
    list(APPEND PLANNER_SOURCES
        pdbs/canonical_pdbs_heuristic
        pdbs/dominance_pruner
        pdbs/match_tree
        pdbs/max_cliques
        pdbs/pattern_database
        pdbs/pattern_generation_edelkamp
        pdbs/pattern_generation_haslum
        pdbs/pdb_heuristic
        pdbs/util
        pdbs/zero_one_pdbs_heuristic
    )
endif()
