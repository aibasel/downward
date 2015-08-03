## == Details of the core source files ==
##
## If you're adding another file to the codebase which *isn't* a plugin, add
## it to the following list. We assume that every *.cc file has a corresponding
## *.h file and add headers to the project automatically.

set(PLANNER_SOURCES
        planner.cc
)

set(CORE_SOURCES
        abstract_task.cc
        axioms.cc
        causal_graph.cc
        cost_adapted_task.cc
        countdown_timer.cc
        delegating_task.cc
        domain_transition_graph.cc
        equivalence_relation.cc
        evaluation_context.cc
        evaluation_result.cc
        exact_timer.cc
        global_operator.cc
        globals.cc
        global_state.cc
        heuristic_cache.cc
        heuristic.cc
        int_packer.cc
        operator_cost.cc
        option_parser.cc
        option_parser_util.cc
        per_state_information.cc
        plugin.cc
        priority_queue.cc
        rng.cc
        root_task.cc
        scalar_evaluator.cc
        search_engine.cc
        search_node_info.cc
        search_progress.cc
        search_space.cc
        search_statistics.cc
        segmented_vector.cc
        state_id.cc
        state_registry.cc
        successor_generator.cc
        task_proxy.cc
        task_tools.cc
        timer.cc
        tracer.cc
        utilities.cc
        utilities_hash.cc
        utilities_windows.cc
        variable_order_finder.cc

        open_lists/alternation_open_list.cc
        open_lists/bucket_open_list.cc
        open_lists/open_list.cc
        open_lists/pareto_open_list.cc
        open_lists/standard_scalar_open_list.cc
        open_lists/tiebreaking_open_list.cc
)

fast_downward_add_headers_to_sources_list(CORE_SOURCES)
source_group(core FILES ${CORE_SOURCES})
list(APPEND PLANNER_SOURCES ${CORE_SOURCES})

## Details of the plugins
#
# For now, everything defaults to being enabled - it's up to the user to specify
#    -DPLUGIN_FOO_ENABLED=FALSE
# to disable a given plugin.
#
# Defining a new plugin:
#    fast_downward_plugin(
#        NAME <NAME>
#        [ DISPLAY_NAME <DISPLAY_NAME> ]
#        [ HELP <HELP> ]
#        SOURCES
#            <FILE 1> [ <FILE 2> ... ]
#        [ DEPENDS <PLUGIN_NAME 1> [ <PLUGIN_NAME 2> ... ] ]
#        [ DEACTIVATED ]
#    )
#
# <DISPLAY_NAME> defaults to lower case <NAME> and is used to group
#                files in IDEs and for messages.
# <HELP> defaults to <DISPLAY_NAME> and is used to describe the cmake option.
# DEPENDS lists plugins that will be automatically enabled if this plugin
# is enabled. If the dependency was not enabled before, this will be logged.
# DEACTIVATED sets the default value of the generated cmake option to false.

fast_downward_plugin(
    NAME G_EVALUATOR
    HELP "The g-evaluator"
    SOURCES
        g_evaluator.cc
)

fast_downward_plugin(
    NAME COMBINING_EVALUATOR
    HELP "The combining evaluator"
    SOURCES
        combining_evaluator.cc
)

fast_downward_plugin(
    NAME MAX_EVALUATOR
    HELP "The max evaluator"
    SOURCES
        max_evaluator.cc
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME PREF_EVALUATOR
    HELP "The pref evaluator"
    SOURCES
        pref_evaluator.cc
)

fast_downward_plugin(
    NAME WEIGHTED_EVALUATOR
    HELP "The weighted evaluator"
    SOURCES
        weighted_evaluator.cc
)

fast_downward_plugin(
    NAME SUM_EVALUATOR
    HELP "The sum evaluator"
    SOURCES
        sum_evaluator.cc
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME EAGER_SEARCH
    HELP "Eager search algorithm"
    SOURCES
        eager_search.cc
    DEPENDS G_EVALUATOR SUM_EVALUATOR
)

fast_downward_plugin(
    NAME LAZY_SEARCH
    HELP "Lazy search algorithm"
    SOURCES
        lazy_search.cc
    DEPENDS G_EVALUATOR SUM_EVALUATOR WEIGHTED_EVALUATOR
)

fast_downward_plugin(
    NAME EHC_SEARCH
    HELP "Enforced hill-climbing search algorithm"
    SOURCES
        enforced_hill_climbing_search.cc
    DEPENDS PREF_EVALUATOR G_EVALUATOR
)

fast_downward_plugin(
    NAME ITERATED_SEARCH
    HELP "Iterated search algorithm"
    SOURCES
        iterated_search.cc
)

fast_downward_plugin(
    NAME LP_SOLVER
    HELP "Interface to an LP solver"
    SOURCES
        lp_internals.cc
        lp_solver.cc
)

fast_downward_plugin(
    NAME RELAXATION_HEURISTIC
    HELP "The base class for relaxation heuristics"
    SOURCES
        relaxation_heuristic.cc
)

fast_downward_plugin(
    NAME IPC_MAX_HEURISTIC
    HELP "The IPC max heuristic"
    SOURCES
        ipc_max_heuristic.cc
)

fast_downward_plugin(
    NAME ADDITIVE_HEURISTIC
    HELP "The additive heuristic"
    SOURCES additive_heuristic.cc
    DEPENDS RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME BLIND_SEARCH_HEURISTIC
    HELP "The 'blind search' heuristic"
    SOURCES blind_search_heuristic.cc
)

fast_downward_plugin(
    NAME CEA_HEURISTIC
    HELP "The context-enhanced additive heuristic"
    SOURCES cea_heuristic.cc
)

fast_downward_plugin(
    NAME CG_HEURISTIC
    HELP "The causal graph heuristic"
    SOURCES cg_heuristic.cc
            cg_cache.cc
)

fast_downward_plugin(
    NAME FF_HEURISTIC
    HELP "The FF heuristic (an implementation of the RPG heuristic)"
    SOURCES ff_heuristic.cc
    DEPENDS ADDITIVE_HEURISTIC
)

fast_downward_plugin(
    NAME GOAL_COUNT_HEURISTIC
    HELP "The goal-counting heuristic"
    SOURCES goal_count_heuristic.cc
)

fast_downward_plugin(
    NAME HM_HEURISTIC
    HELP "The h^m heuristic"
    SOURCES hm_heuristic.cc
)

fast_downward_plugin(
    NAME LM_CUT_HEURISTIC
    HELP "The LM-cut heuristic"
    SOURCES lm_cut_heuristic.cc
)

fast_downward_plugin(
    NAME MAX_HEURISTIC
    HELP "The Max heuristic"
    SOURCES max_heuristic.cc
    DEPENDS RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME MAS_HEURISTIC
    HELP "The Merge-and-Shrink heuristic"
    SOURCES
        merge_and_shrink/distances.cc
        merge_and_shrink/factored_transition_system.cc
        merge_and_shrink/fts_factory.cc
        merge_and_shrink/heuristic_representation.cc
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

fast_downward_plugin(
    NAME LANDMARKS
    HELP "Plugin containing the code to reason with landmarks"
    SOURCES
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
        landmarks/util.cc
    DEPENDS LP_SOLVER
)

fast_downward_plugin(
    NAME LEARNING
    HELP "Plugin containing the code to reason with learning"
    SOURCES
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
    DEACTIVATED
)

fast_downward_plugin(
    NAME PDBS
    HELP "Plugin containing the code for PDBs"
    SOURCES
        pdbs/canonical_pdbs_heuristic.cc
        pdbs/dominance_pruner.cc
        pdbs/match_tree.cc
        pdbs/max_cliques.cc
        pdbs/pattern_database.cc
        pdbs/pattern_generation_edelkamp.cc
        pdbs/pattern_generation_haslum.cc
        pdbs/pdb_heuristic.cc
        pdbs/util.cc
        pdbs/zero_one_pdbs_heuristic.cc
)

fast_downward_add_plugin_sources(PLANNER_SOURCES)

