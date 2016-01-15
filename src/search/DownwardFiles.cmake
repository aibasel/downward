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
        delegating_task.cc
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
        option_parser.h
        option_parser_util.h
        per_state_information.cc
        plugin.h
        pruning_method.cc
        priority_queue.cc
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
        task_proxy.cc
        task_tools.cc
        variable_order_finder.cc

        open_lists/alternation_open_list.cc
        open_lists/bucket_open_list.cc
        open_lists/epsilon_greedy_open_list.cc
        open_lists/open_list.cc
        open_lists/open_list_factory.cc
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
#        [ CORE_PLUGIN ]
#    )
#
# <DISPLAY_NAME> defaults to lower case <NAME> and is used to group
#                files in IDEs and for messages.
# <HELP> defaults to <DISPLAY_NAME> and is used to describe the cmake option.
# DEPENDS lists plugins that will be automatically enabled if this plugin
# is enabled. If the dependency was not enabled before, this will be logged.
# DEPENDENCY_ONLY disables the plugin unless it is needed as a dependency and
#     hides the option to enable the plugin in cmake GUIs like ccmake.
# CORE_PLUGIN enables the plugin and hides the option to disable it in
#     cmake GUIs like ccmake.

option(
    DISABLE_PLUGINS_BY_DEFAULT
    "If set to YES only plugins that are specifically enabled will be compiled"
    NO)
# This option should not show up in cmake GUIs like ccmake where all
# plugins are enabled or disabled manually.
mark_as_advanced(DISABLE_PLUGINS_BY_DEFAULT)

fast_downward_plugin(
    NAME OPTIONS
    HELP "Option parsing and plugin definition"
    SOURCES
        options/any.h
        options/bounds.cc
        options/doc_printer.cc
        options/doc_store.cc
        options/errors.cc
        options/option_parser.cc
        options/options.cc
        options/parse_tree.cc
        options/predefinitions.cc
        options/plugin.cc
        options/registries.cc
        options/synergy.cc
        options/token_parser.cc
        options/type_documenter.cc
        options/type_namer.cc
    CORE_PLUGIN
)

fast_downward_plugin(
    NAME UTILS
    HELP "System utilities"
    SOURCES
        utils/collections.h
        utils/countdown_timer.cc
        utils/dynamic_bitset.h
        utils/hash.h
        utils/language.h
        utils/logging.cc
        utils/markup.cc
        utils/math.cc
        utils/memory.cc
        utils/rng.cc
        utils/system.cc
        utils/system_unix.cc
        utils/system_windows.cc
        utils/timer.cc
    CORE_PLUGIN
)

fast_downward_plugin(
    NAME CONST_EVALUATOR
    HELP "The constant evaluator"
    SOURCES
        evaluators/const_evaluator.cc
)

fast_downward_plugin(
    NAME G_EVALUATOR
    HELP "The g-evaluator"
    SOURCES
        evaluators/g_evaluator.cc
)

fast_downward_plugin(
    NAME COMBINING_EVALUATOR
    HELP "The combining evaluator"
    SOURCES
        evaluators/combining_evaluator.cc
)

fast_downward_plugin(
    NAME MAX_EVALUATOR
    HELP "The max evaluator"
    SOURCES
        evaluators/max_evaluator.cc
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME PREF_EVALUATOR
    HELP "The pref evaluator"
    SOURCES
        evaluators/pref_evaluator.cc
)

fast_downward_plugin(
    NAME WEIGHTED_EVALUATOR
    HELP "The weighted evaluator"
    SOURCES
        evaluators/weighted_evaluator.cc
)

fast_downward_plugin(
    NAME SUM_EVALUATOR
    HELP "The sum evaluator"
    SOURCES
        evaluators/sum_evaluator.cc
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME NULL_PRUNING_METHOD
    HELP "Pruning method that does nothing"
    SOURCES
        pruning/null_pruning_method.cc
)

fast_downward_plugin(
    NAME STUBBORN_SETS
    HELP "Base class for all stubborn set partial order reduction methods"
    SOURCES
        pruning/stubborn_sets.cc
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME STUBBORN_SETS_SIMPLE
    HELP "Stubborn sets simple"
    SOURCES
        pruning/stubborn_sets_simple.cc
    DEPENDS STUBBORN_SETS
)

fast_downward_plugin(
    NAME StubbornSetsEC
    HELP "Stubborn set method that dominates expansion core"
    SOURCES
        pruning/stubborn_sets_ec.cc
    DEPENDS STUBBORN_SETS
)

fast_downward_plugin(
    NAME SEARCH_COMMON
    HELP "Basic classes used for all search engines"
    SOURCES
        search_engines/search_common.cc
    DEPENDS G_EVALUATOR SUM_EVALUATOR WEIGHTED_EVALUATOR
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME EAGER_SEARCH
    HELP "Eager search algorithm"
    SOURCES
        search_engines/eager_search.cc
    DEPENDS SEARCH_COMMON NULL_PRUNING_METHOD
)

fast_downward_plugin(
    NAME ENFORCED_HILL_CLIMBING_SEARCH
    HELP "Lazy enforced hill-climbing search algorithm"
    SOURCES
        search_engines/enforced_hill_climbing_search.cc
    DEPENDS SEARCH_COMMON PREF_EVALUATOR G_EVALUATOR
)

fast_downward_plugin(
    NAME ITERATED_SEARCH
    HELP "Iterated search algorithm"
    SOURCES
        search_engines/iterated_search.cc
)

fast_downward_plugin(
    NAME LAZY_SEARCH
    HELP "Lazy search algorithm"
    SOURCES
        search_engines/lazy_search.cc
    DEPENDS SEARCH_COMMON
)

fast_downward_plugin(
    NAME LP_SOLVER
    HELP "Interface to an LP solver"
    SOURCES
        lp/lp_internals.cc
        lp/lp_solver.cc
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME RELAXATION_HEURISTIC
    HELP "The base class for relaxation heuristics"
    SOURCES
        heuristics/relaxation_heuristic.cc
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME IPC_MAX_HEURISTIC
    HELP "The IPC max heuristic"
    SOURCES
        heuristics/ipc_max_heuristic.cc
)

fast_downward_plugin(
    NAME ADDITIVE_HEURISTIC
    HELP "The additive heuristic"
    SOURCES
        heuristics/additive_heuristic.cc
    DEPENDS RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME BLIND_SEARCH_HEURISTIC
    HELP "The 'blind search' heuristic"
    SOURCES
        heuristics/blind_search_heuristic.cc
)

fast_downward_plugin(
    NAME CONTEXT_ENHANCED_ADDITIVE_HEURISTIC
    HELP "The context-enhanced additive heuristic"
    SOURCES
        heuristics/cea_heuristic.cc
    DEPENDS DOMAIN_TRANSITION_GRAPH
)

fast_downward_plugin(
    NAME CG_HEURISTIC
    HELP "The causal graph heuristic"
    SOURCES heuristics/cg_heuristic.cc
            heuristics/cg_cache.cc
    DEPENDS DOMAIN_TRANSITION_GRAPH
)

fast_downward_plugin(
    NAME DOMAIN_TRANSITION_GRAPH
    HELP "DTGs used by cg and cea heuristic"
    SOURCES
        domain_transition_graph.cc
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME FF_HEURISTIC
    HELP "The FF heuristic (an implementation of the RPG heuristic)"
    SOURCES
        heuristics/ff_heuristic.cc
    DEPENDS ADDITIVE_HEURISTIC
)

fast_downward_plugin(
    NAME GOAL_COUNT_HEURISTIC
    HELP "The goal-counting heuristic"
    SOURCES
        heuristics/goal_count_heuristic.cc
)

fast_downward_plugin(
    NAME HM_HEURISTIC
    HELP "The h^m heuristic"
    SOURCES
        heuristics/hm_heuristic.cc
)

fast_downward_plugin(
    NAME LANDMARK_CUT_HEURISTIC
    HELP "The LM-cut heuristic"
    SOURCES
        heuristics/lm_cut_heuristic.cc
        heuristics/lm_cut_landmarks.cc
)

fast_downward_plugin(
    NAME MAX_HEURISTIC
    HELP "The Max heuristic"
    SOURCES
        heuristics/max_heuristic.cc
    DEPENDS RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME EXTRA_TASKS
    HELP "Non-core task transformations"
    SOURCES
        tasks/domain_abstracted_task.cc
        tasks/domain_abstracted_task_factory.cc
        tasks/modified_goals_task.cc
        tasks/modified_operator_costs_task.cc
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME CEGAR
    HELP "Plugin containing the code for CEGAR heuristics"
    SOURCES
        cegar/abstraction.cc
        cegar/abstract_search.cc
        cegar/abstract_state.cc
        cegar/additive_cartesian_heuristic.cc
        cegar/cartesian_heuristic.cc
        cegar/domains.cc
        cegar/refinement_hierarchy.cc
        cegar/split_selector.cc
        cegar/subtask_generators.cc
        cegar/utils.cc
        cegar/utils_landmarks.cc
    DEPENDS EXTRA_TASKS
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
        merge_and_shrink/label_reduction.cc
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
        merge_and_shrink/types.cc
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
    DEPENDS LP_SOLVER LANDMARK_CUT_HEURISTIC PDBS
)

fast_downward_plugin(
    NAME PDBS
    HELP "Plugin containing the code for PDBs"
    SOURCES
        pdbs/canonical_pdbs.cc
        pdbs/canonical_pdbs_heuristic.cc
        pdbs/dominance_pruning.cc
        pdbs/incremental_canonical_pdbs.cc
        pdbs/match_tree.cc
        pdbs/max_additive_pdb_sets.cc
        pdbs/max_cliques.cc
        pdbs/pattern_collection_information.cc
        pdbs/pattern_database.cc
        pdbs/pattern_collection_generator_combo.cc
        pdbs/pattern_collection_generator_genetic.cc
        pdbs/pattern_collection_generator_hillclimbing.cc
        pdbs/pattern_collection_generator_manual.cc
        pdbs/pattern_collection_generator_systematic.cc
        pdbs/pattern_generator_greedy.cc
        pdbs/pattern_generator_manual.cc
        pdbs/pattern_generator.cc
        pdbs/pdb_heuristic.cc
        pdbs/types.cc
        pdbs/validation.cc
        pdbs/zero_one_pdbs.cc
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
