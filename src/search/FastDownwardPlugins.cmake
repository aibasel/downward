## == Details of the core source files ==
##
## If you're adding another file to the codebase which *isn't* a plugin, add
## it to the following list

set(PLANNER_SOURCES
        planner.cc

        abstract_task.cc abstract_task.h
        axioms.cc axioms.h
        causal_graph.cc causal_graph.h
        cost_adapted_task.cc cost_adapted_task.h
        countdown_timer.cc countdown_timer.h
        delegating_task.cc delegating_task.h
        doc.h # TODO: unused?
        domain_transition_graph.cc domain_transition_graph.h
        equivalence_relation.cc equivalence_relation.h
        evaluation_context.cc evaluation_context.h
        evaluation_result.cc evaluation_result.h
        exact_timer.cc exact_timer.h
        global_operator.cc global_operator.h
        globals.cc globals.h
        global_state.cc global_state.h
        heuristic_cache.cc heuristic_cache.h
        heuristic.cc heuristic.h
        int_packer.cc int_packer.h
        operator_cost.cc operator_cost.h
        option_parser.cc option_parser.h
        option_parser_util.cc option_parser_util.h
        per_state_information.cc per_state_information.h
        plugin.h
        priority_queue.h
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
        task_proxy.cc task_proxy.h
        task_tools.cc task_tools.h
        timer.cc timer.h
        tracer.cc tracer.h
        utilities.cc utilities.h
        utilities_hash.cc utilities_hash.h
        utilities_windows.h
        variable_order_finder.cc variable_order_finder.h

        open_lists/alternation_open_list.cc open_lists/alternation_open_list.h
        open_lists/bucket_open_list.cc open_lists/bucket_open_list.h
        open_lists/open_list.cc open_lists/open_list.h
        open_lists/pareto_open_list.cc open_lists/pareto_open_list.h
        open_lists/standard_scalar_open_list.cc open_lists/standard_scalar_open_list.h
        open_lists/tiebreaking_open_list.cc open_lists/tiebreaking_open_list.h
)

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
        g_evaluator.cc g_evaluator.h
)

fast_downward_plugin(
    NAME COMBINING_EVALUATOR
    HELP "The combining evaluator"
    SOURCES
        combining_evaluator.cc combining_evaluator.h
)

fast_downward_plugin(
    NAME MAX_EVALUATOR
    HELP "The max evaluator"
    SOURCES
        max_evaluator.cc max_evaluator.h
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME PREF_EVALUATOR
    HELP "The pref evaluator"
    SOURCES
        pref_evaluator.cc pref_evaluator.h
)

fast_downward_plugin(
    NAME WEIGHTED_EVALUATOR
    HELP "The weighted evaluator"
    SOURCES
        weighted_evaluator.cc weighted_evaluator.h
)

fast_downward_plugin(
    NAME SUM_EVALUATOR
    HELP "The sum evaluator"
    SOURCES
        sum_evaluator.cc sum_evaluator.h
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME EAGER_SEARCH
    HELP "Eager search algorithm"
    SOURCES
        eager_search.cc eager_search.h
    DEPENDS G_EVALUATOR SUM_EVALUATOR
)

fast_downward_plugin(
    NAME LAZY_SEARCH
    HELP "Lazy search algorithm"
    SOURCES
        lazy_search.cc lazy_search.h
    DEPENDS G_EVALUATOR SUM_EVALUATOR WEIGHTED_EVALUATOR
)

fast_downward_plugin(
    NAME EHC_SEARCH
    HELP "Enforced hill-climbing search algorithm"
    SOURCES
        enforced_hill_climbing_search.cc enforced_hill_climbing_search.h
    DEPENDS PREF_EVALUATOR G_EVALUATOR
)

fast_downward_plugin(
    NAME ITERATED_SEARCH
    HELP "Iterated search algorithm"
    SOURCES
        iterated_search.cc iterated_search.h
)

fast_downward_plugin(
    NAME LP_SOLVER
    HELP "Interface to an LP solver"
    SOURCES
        lp_internals.cc lp_internals.h
        lp_solver.cc lp_solver.h
)

fast_downward_plugin(
    NAME RELAXATION_HEURISTIC
    HELP "The base class for relaxation heuristics"
    SOURCES
        relaxation_heuristic.cc relaxation_heuristic.h
)

fast_downward_plugin(
    NAME IPC_MAX_HEURISTIC
    HELP "The IPC max heuristic"
    SOURCES
        ipc_max_heuristic.cc ipc_max_heuristic.h
)

fast_downward_plugin(
    NAME ADDITIVE_HEURISTIC
    HELP "The additive heuristic"
    SOURCES additive_heuristic.cc additive_heuristic.h
    DEPENDS RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME BLIND_SEARCH_HEURISTIC
    HELP "The 'blind search' heuristic"
    SOURCES blind_search_heuristic.cc blind_search_heuristic.h
)

fast_downward_plugin(
    NAME CEA_HEURISTIC
    HELP "The context-enhanced additive heuristic"
    SOURCES cea_heuristic.cc cea_heuristic.h
)

fast_downward_plugin(
    NAME CG_HEURISTIC
    HELP "The causal graph heuristic"
    SOURCES cg_heuristic.cc cg_heuristic.h
            cg_cache.cc cg_cache.h
)

fast_downward_plugin(
    NAME FF_HEURISTIC
    HELP "The FF heuristic (an implementation of the RPG heuristic)"
    SOURCES ff_heuristic.cc ff_heuristic.h
    DEPENDS ADDITIVE_HEURISTIC
)

fast_downward_plugin(
    NAME GOAL_COUNT_HEURISTIC
    HELP "The goal-counting heuristic"
    SOURCES goal_count_heuristic.cc goal_count_heuristic.h
)

fast_downward_plugin(
    NAME HM_HEURISTIC
    HELP "The h^m heuristic"
    SOURCES hm_heuristic.cc hm_heuristic.h
)

fast_downward_plugin(
    NAME LM_CUT_HEURISTIC
    HELP "The LM-cut heuristic"
    SOURCES lm_cut_heuristic.cc lm_cut_heuristic.h
)

fast_downward_plugin(
    NAME MAX_HEURISTIC
    HELP "The Max heuristic"
    SOURCES max_heuristic.cc max_heuristic.h
    DEPENDS RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME MAS_HEURISTIC
    HELP "The Merge-and-Shrink heuristic"
    SOURCES
        merge_and_shrink/distances.cc merge_and_shrink/distances.h
        merge_and_shrink/factored_transition_system.cc merge_and_shrink/factored_transition_system.h
        merge_and_shrink/fts_factory.cc merge_and_shrink/fts_factory.h
        merge_and_shrink/heuristic_representation.cc merge_and_shrink/heuristic_representation.h
        merge_and_shrink/labels.cc merge_and_shrink/labels.h
        merge_and_shrink/merge_and_shrink_heuristic.cc merge_and_shrink/merge_and_shrink_heuristic.h
        merge_and_shrink/merge_dfp.cc merge_and_shrink/merge_dfp.h
        merge_and_shrink/merge_linear.cc merge_and_shrink/merge_linear.h
        merge_and_shrink/merge_strategy.cc merge_and_shrink/merge_strategy.h
        merge_and_shrink/shrink_bisimulation.cc merge_and_shrink/shrink_bisimulation.h
        merge_and_shrink/shrink_bucket_based.cc merge_and_shrink/shrink_bucket_based.h
        merge_and_shrink/shrink_fh.cc merge_and_shrink/shrink_fh.h
        merge_and_shrink/shrink_random.cc merge_and_shrink/shrink_random.h
        merge_and_shrink/shrink_strategy.cc merge_and_shrink/shrink_strategy.h
        merge_and_shrink/transition_system.cc merge_and_shrink/transition_system.h
)

fast_downward_plugin(
    NAME LANDMARKS
    HELP "Plugin containing the code to reason with landmarks"
    SOURCES
        landmarks/exploration.cc landmarks/exploration.h
        landmarks/h_m_landmarks.cc landmarks/h_m_landmarks.h
        landmarks/lama_ff_synergy.cc landmarks/lama_ff_synergy.h
        landmarks/landmark_cost_assignment.cc landmarks/landmark_cost_assignment.h
        landmarks/landmark_count_heuristic.cc landmarks/landmark_count_heuristic.h
        landmarks/landmark_factory.cc landmarks/landmark_factory.h
        landmarks/landmark_factory_rpg_exhaust.cc landmarks/landmark_factory_rpg_exhaust.h
        landmarks/landmark_factory_rpg_sasp.cc landmarks/landmark_factory_rpg_sasp.h
        landmarks/landmark_factory_zhu_givan.cc landmarks/landmark_factory_zhu_givan.h
        landmarks/landmark_graph.cc landmarks/landmark_graph.h
        landmarks/landmark_graph_merged.cc landmarks/landmark_graph_merged.h
        landmarks/landmark_status_manager.cc landmarks/landmark_status_manager.h
        landmarks/landmark_types.h
        landmarks/util.cc landmarks/util.h
    DEPENDS LP_SOLVER
)

fast_downward_plugin(
    NAME LEARNING
    HELP "Plugin containing the code to reason with learning"
    SOURCES
        learning/AODE.cc learning/AODE.h
        learning/classifier.cc learning/classifier.h
        learning/composite_feature_extractor.cc learning/composite_feature_extractor.h
        learning/feature_extractor.cc learning/feature_extractor.h
        learning/maximum_heuristic.cc learning/maximum_heuristic.h
        learning/naive_bayes_classifier.cc learning/naive_bayes_classifier.h
        learning/PDB_state_space_sample.cc learning/PDB_state_space_sample.h
        learning/probe_state_space_sample.cc learning/probe_state_space_sample.h
        learning/selective_max_heuristic.cc learning/selective_max_heuristic.h
        learning/state_space_sample.cc learning/state_space_sample.h
        learning/state_vars_feature_extractor.cc learning/state_vars_feature_extractor.h
    DEACTIVATED
)

fast_downward_plugin(
    NAME PDBS
    HELP "Plugin containing the code for PDBs"
    SOURCES
        pdbs/canonical_pdbs_heuristic.cc pdbs/canonical_pdbs_heuristic.h
        pdbs/dominance_pruner.cc pdbs/dominance_pruner.h
        pdbs/match_tree.cc pdbs/match_tree.h
        pdbs/max_cliques.cc pdbs/max_cliques.h
        pdbs/pattern_database.cc pdbs/pattern_database.h
        pdbs/pattern_generation_edelkamp.cc pdbs/pattern_generation_edelkamp.h
        pdbs/pattern_generation_haslum.cc pdbs/pattern_generation_haslum.h
        pdbs/pdb_heuristic.cc pdbs/pdb_heuristic.h
        pdbs/util.cc pdbs/util.h
        pdbs/zero_one_pdbs_heuristic.cc pdbs/zero_one_pdbs_heuristic.h
)

fast_downward_add_plugin_sources(PLANNER_SOURCES)
