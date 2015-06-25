## == Details of the core source files ==
##
## If you're adding another file to the codebase which *isn't* a plugin, add
## it to the following list

set(search_main_SRCS
        abstract_task
        axioms
        causal_graph
        combining_evaluator
        cost_adapted_task
        countdown_timer
        delegating_task
        doc
        domain_transition_graph
        eager_search
        enforced_hill_climbing_search
        equivalence_relation
        evaluation_context
        evaluation_result
        exact_timer
        g_evaluator
        global_operator
        globals
        global_state
        heuristic_cache
        heuristic
        int_packer
        ipc_max_heuristic
        iterated_search
        lazy_search
        lp_internals
        lp_solver
        max_evaluator
        operator_cost
        option_parser
        option_parser_util
        per_state_information
        plugin
        pref_evaluator
        priority_queue
        relaxation_heuristic
        rng
        root_task
        scalar_evaluator
        search_engine
        search_node_info
        search_progress
        search_space
        search_statistics
        segmented_vector
        state_id
        state_registry
        successor_generator
        sum_evaluator
        task_proxy
        task_tools
        timer
        tracer
        utilities
        utilities_hash
        variable_order_finder
        weighted_evaluator
        open_lists/alternation_open_list
        open_lists/bucket_open_list
        open_lists/open_list
        open_lists/pareto_open_list
        open_lists/standard_scalar_open_list
        open_lists/tiebreaking_open_list
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
    set(search_plugin_SRCS ${search_plugin_SRCS} additive_heuristic)
endif()


option(PLUGIN_BLIND_SEARCH_HEURISTIC "The 'blind search' heuristic" TRUE)
if(PLUGIN_BLIND_SEARCH_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} blind_search_heuristic)
endif()


option(PLUGIN_CEA_HEURISTIC "The context-enhanced additive heuristic" TRUE)
if(PLUGIN_CEA_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} cea_heuristic)
endif()


option(PLUGIN_CG_HEURISTIC "The causal graph heuristic" TRUE)
if(PLUGIN_CG_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} cg_heuristic cg_cache)
endif()


option(PLUGIN_FF_HEURISTIC "The FF heuristic (an implementation of the RPG heuristic)" TRUE)
if(PLUGIN_FF_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} ff_heuristic)
endif()


option(PLUGIN_GOAL_COUNT_HEURISTIC "The goal-counting heuristic" TRUE)
if(PLUGIN_GOAL_COUNT_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} goal_count_heuristic)
endif()


option(PLUGIN_HM_HEURISTIC "The h^m heuristic" TRUE)
if(PLUGIN_HM_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} hm_heuristic)
endif()


option(PLUGIN_LM_CUT_HEURISTIC "The LM-cut heuristic" TRUE)
if(PLUGIN_LM_CUT_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} lm_cut_heuristic)
endif()


option(PLUGIN_MAX_HEURISTIC "The Max heuristic" TRUE)
if(PLUGIN_MAX_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} max_heuristic)
endif()


option(PLUGIN_MAS_HEURISTIC "The Merge-and-Shrink heuristic" TRUE)
if(PLUGIN_MAS_HEURISTIC)
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
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
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
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
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
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
    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        pdbs/canonical_pdbs_heuristic
        pdbs/dominance_pruner
        pdbs/match_tree
        pdbs/max_cliques
        pdbs/pattern_generation_edelkamp
        pdbs/pattern_generation_haslum
        pdbs/pdb_heuristic
        pdbs/util
        pdbs/zero_one_pdbs_heuristic
    )
endif()
