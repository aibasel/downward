set(PLANNER_SOURCES
        planner.cc
)

# If you're adding a file to the codebase which *isn't* a plugin, add
# it to the following list. We assume that every *.cc file has a
# corresponding *.h file and add headers to the project automatically.
# For plugin files, see below.

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
        sampling.cc
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
        system.cc
        system_unix.cc
        system_windows.cc
        task_proxy.cc
        task_tools.cc
        timer.cc
        tracer.cc
        utilities.cc
        utilities_hash.cc
        variable_order_finder.cc

        open_lists/alternation_open_list.cc
        open_lists/bucket_open_list.cc
        open_lists/epsilon_greedy_open_list.cc
        open_lists/open_list.cc
        open_lists/pareto_open_list.cc
        open_lists/standard_scalar_open_list.cc
        open_lists/tiebreaking_open_list.cc
        open_lists/type_based_open_list.cc
)

fast_downward_add_headers_to_sources_list(CORE_SOURCES)
source_group(core FILES planner.cc ${CORE_SOURCES})
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
#            <FILE_1> [ <FILE_2> ... ]
#        [ DEPENDS <PLUGIN_NAME_1> [ <PLUGIN_NAME_2> ... ] ]
#        [ DEPENDENCY_ONLY ]
#    )
#
# <DISPLAY_NAME> defaults to lower case <NAME> and is used to group
#                files in IDEs and for messages.
# <HELP> defaults to <DISPLAY_NAME> and is used to describe the cmake option.
# DEPENDS lists plugins that will be automatically enabled if this plugin
# is enabled. If the dependency was not enabled before, this will be logged.
# DEPENDENCY_ONLY disables the plugin unless it is needed as a dependency and
#     hides the option to enable the plugin in cmake GUIs like ccmake.

fast_downward_plugin(
    NAME CONST_EVALUATOR
    HELP "The constant evaluator"
    SOURCES
        const_evaluator.cc
)

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
    HELP "Lazy enforced hill-climbing search algorithm"
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
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME RELAXATION_HEURISTIC
    HELP "The base class for relaxation heuristics"
    SOURCES
        relaxation_heuristic.cc
    DEPENDENCY_ONLY
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
    SOURCES
        lm_cut_heuristic.cc
        lm_cut_landmarks.cc
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
        merge_and_shrink/label_equivalence_relation.cc
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
    NAME OPERATOR_COUNTING
    HELP "Plugin containing the code for operator counting heuristics"
    SOURCES
        operator_counting/constraint_generator.cc
        operator_counting/lm_cut_constraints.cc
        operator_counting/operator_counting_heuristic.cc
        operator_counting/pho_constraints.cc
        operator_counting/state_equation_constraints.cc
    DEPENDS LP_SOLVER LM_CUT_HEURISTIC PDBS
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
        pdbs/pattern_generation_systematic.cc
        pdbs/pdb_heuristic.cc
        pdbs/util.cc
        pdbs/zero_one_pdbs_heuristic.cc
)

fast_downward_plugin(
    NAME POTENTIALS
    HELP "Plugin containing the code for potential heuristics"
    SOURCES
        potentials/diverse_potential_heuristics.cc
        potentials/potential_function.cc
        potentials/potential_heuristic.cc
        potentials/potential_max_heuristic.cc
        potentials/potential_optimizer.cc
        potentials/sample_based_potential_heuristics.cc
        potentials/single_potential_heuristics.cc
        potentials/util.cc
    DEPENDS LP_SOLVER
)

fast_downward_add_plugin_sources(PLANNER_SOURCES)

# The order in PLANNER_SOURCES influences the order in which object
# files are given to the linker, which can have a significant influence
# on performance (see issue67). The general recommendation seems to be
# to list files that define functions after files that use them.
# We approximate this by reversing the list, which will put the plugins
# first, followed by the core files, followed by the main file.
# This is certainly not optimal, but works well enough in practice.
list(REVERSE PLANNER_SOURCES)
