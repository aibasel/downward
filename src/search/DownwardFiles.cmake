# See http://www.fast-downward.org/ForDevelopers/AddingSourceFiles
# for general information on adding source files and CMake plugins.
#
# All plugins are enabled by default and users can disable them by specifying
#    -DPLUGIN_FOO_ENABLED=FALSE
# The default behavior can be changed so all non-essential plugins are
# disabled by default by specifying
#    -DDISABLE_PLUGINS_BY_DEFAULT=TRUE
# In that case, individual plugins can be enabled with
#    -DPLUGIN_FOO_ENABLED=TRUE
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
# <DISPLAY_NAME> defaults to lower case <NAME> and is used to group files
#   in IDEs and for messages.
# <HELP> defaults to <DISPLAY_NAME> and is used to describe the cmake option.
# SOURCES lists the source files that are part of the plugin. Entries are
#   listed without extension. For an entry <file>, both <file>.h and <file>.cc
#   are added if the files exist.
# DEPENDS lists plugins that will be automatically enabled if this plugin is
#   enabled. If the dependency was not enabled before, this will be logged.
# DEPENDENCY_ONLY disables the plugin unless it is needed as a dependency and
#   hides the option to enable the plugin in cmake GUIs like ccmake.
# CORE_PLUGIN always enables the plugin (even if DISABLE_PLUGINS_BY_DEFAULT
#   is used) and hides the option to disable it in CMake GUIs like ccmake.

option(
    DISABLE_PLUGINS_BY_DEFAULT
    "If set to YES only plugins that are specifically enabled will be compiled"
    NO)
# This option should not show up in CMake GUIs like ccmake where all
# plugins are enabled or disabled manually.
mark_as_advanced(DISABLE_PLUGINS_BY_DEFAULT)

fast_downward_plugin(
    NAME CORE_SOURCES
    HELP "Core source files"
    SOURCES
        planner

        abstract_task
        axioms
        evaluation_context
        evaluation_result
        evaluator
        global_operator
        globals
        global_state
        heuristic_cache
        heuristic
        open_list
        open_list_factory
        operator_cost
        operator_id
        option_parser
        option_parser_util
        per_state_information
        plugin
        pruning_method
        search_engine
        search_node_info
        search_progress
        search_space
        search_statistics
        state_id
        state_registry
        task_proxy

    DEPENDS CAUSAL_GRAPH INT_PACKER ORDERED_SET SEGMENTED_VECTOR SUCCESSOR_GENERATOR TASK_PROPERTIES
    CORE_PLUGIN
)

fast_downward_plugin(
    NAME OPTIONS
    HELP "Option parsing and plugin definition"
    SOURCES
        options/any
        options/bounds
        options/doc_printer
        options/doc_store
        options/errors
        options/option_parser
        options/options
        options/parse_tree
        options/predefinitions
        options/plugin
        options/registries
        options/synergy
        options/token_parser
        options/type_namer
    CORE_PLUGIN
)

fast_downward_plugin(
    NAME UTILS
    HELP "System utilities"
    SOURCES
        utils/collections
        utils/countdown_timer
        utils/hash
        utils/language
        utils/logging
        utils/markup
        utils/math
        utils/memory
        utils/rng
        utils/rng_options
        utils/system
        utils/system_unix
        utils/system_windows
        utils/timer
    CORE_PLUGIN
)

fast_downward_plugin(
    NAME ALTERNATION_OPEN_LIST
    HELP "Open list that alternates between underlying open lists in a round-robin manner"
    SOURCES
        open_lists/alternation_open_list
)

fast_downward_plugin(
    NAME EPSILON_GREEDY_OPEN_LIST
    HELP "Open list that chooses an entry randomly with probability epsilon"
    SOURCES
        open_lists/epsilon_greedy_open_list
)

fast_downward_plugin(
    NAME PARETO_OPEN_LIST
    HELP "Pareto open list"
    SOURCES
        open_lists/pareto_open_list
)

fast_downward_plugin(
    NAME STANDARD_SCALAR_OPEN_LIST
    HELP "Standard scalar open list"
    SOURCES
        open_lists/standard_scalar_open_list
)

fast_downward_plugin(
    NAME TIEBREAKING_OPEN_LIST
    HELP "Tiebreaking open list"
    SOURCES
        open_lists/tiebreaking_open_list
)

fast_downward_plugin(
    NAME TYPE_BASED_OPEN_LIST
    HELP "Type-based open list"
    SOURCES
        open_lists/type_based_open_list
)

fast_downward_plugin(
    NAME DYNAMIC_BITSET
    HELP "Poor man's version of boost::dynamic_bitset"
    SOURCES
        algorithms/dynamic_bitset
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME EQUIVALENCE_RELATION
    HELP "Equivalence relation over [1, ..., n] that can be iteratively refined"
    SOURCES
        algorithms/equivalence_relation
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME INT_PACKER
    HELP "Greedy bin packing algorithm to pack integer variables with small domains tightly into memory"
    SOURCES
        algorithms/int_packer
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME MAX_CLIQUES
    HELP "Implementation of the Max Cliques algorithm by Tomita et al."
    SOURCES
        algorithms/max_cliques
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME PRIORITY_QUEUES
    HELP "Three implementations of priority queue: HeapQueue, BucketQueue and AdaptiveQueue"
    SOURCES
        algorithms/priority_queues
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME ORDERED_SET
    HELP "Set of elements ordered by insertion time"
    SOURCES
        algorithms/ordered_set
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME SEGMENTED_VECTOR
    HELP "Memory-friendly and vector-like data structure"
    SOURCES
        algorithms/segmented_vector
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME CONST_EVALUATOR
    HELP "The constant evaluator"
    SOURCES
        evaluators/const_evaluator
)

fast_downward_plugin(
    NAME G_EVALUATOR
    HELP "The g-evaluator"
    SOURCES
        evaluators/g_evaluator
)

fast_downward_plugin(
    NAME COMBINING_EVALUATOR
    HELP "The combining evaluator"
    SOURCES
        evaluators/combining_evaluator
)

fast_downward_plugin(
    NAME MAX_EVALUATOR
    HELP "The max evaluator"
    SOURCES
        evaluators/max_evaluator
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME PREF_EVALUATOR
    HELP "The pref evaluator"
    SOURCES
        evaluators/pref_evaluator
)

fast_downward_plugin(
    NAME WEIGHTED_EVALUATOR
    HELP "The weighted evaluator"
    SOURCES
        evaluators/weighted_evaluator
)

fast_downward_plugin(
    NAME SUM_EVALUATOR
    HELP "The sum evaluator"
    SOURCES
        evaluators/sum_evaluator
    DEPENDS COMBINING_EVALUATOR
)

fast_downward_plugin(
    NAME NULL_PRUNING_METHOD
    HELP "Pruning method that does nothing"
    SOURCES
        pruning/null_pruning_method
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME STUBBORN_SETS
    HELP "Base class for all stubborn set partial order reduction methods"
    SOURCES
        pruning/stubborn_sets
    DEPENDS TASK_PROPERTIES
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME STUBBORN_SETS_SIMPLE
    HELP "Stubborn sets simple"
    SOURCES
        pruning/stubborn_sets_simple
    DEPENDS STUBBORN_SETS
)

fast_downward_plugin(
    NAME StubbornSetsEC
    HELP "Stubborn set method that dominates expansion core"
    SOURCES
        pruning/stubborn_sets_ec
    DEPENDS STUBBORN_SETS TASK_PROPERTIES
)

fast_downward_plugin(
    NAME SEARCH_COMMON
    HELP "Basic classes used for all search engines"
    SOURCES
        search_engines/search_common
    DEPENDS ALTERNATION_OPEN_LIST G_EVALUATOR STANDARD_SCALAR_OPEN_LIST SUM_EVALUATOR TIEBREAKING_OPEN_LIST WEIGHTED_EVALUATOR
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME EAGER_SEARCH
    HELP "Eager search algorithm"
    SOURCES
        search_engines/eager_search
    DEPENDS NULL_PRUNING_METHOD ORDERED_SET SUCCESSOR_GENERATOR
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME PLUGIN_ASTAR
    HELP "A* search"
    SOURCES
        search_engines/plugin_astar
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

fast_downward_plugin(
    NAME PLUGIN_EAGER
    HELP "Eager (i.e., normal) best-first search"
    SOURCES
        search_engines/plugin_eager
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

fast_downward_plugin(
    NAME PLUGIN_EAGER_GREEDY
    HELP "Eager greedy best-first search"
    SOURCES
        search_engines/plugin_eager_greedy
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

fast_downward_plugin(
    NAME PLUGIN_LAZY
    HELP "Best-first search with deferred evaluation (lazy)"
    SOURCES
        search_engines/plugin_lazy
    DEPENDS LAZY_SEARCH SEARCH_COMMON
)

fast_downward_plugin(
    NAME PLUGIN_LAZY_GREEDY
    HELP "Greedy best-first search with deferred evaluation (lazy)"
    SOURCES
        search_engines/plugin_lazy_greedy
    DEPENDS LAZY_SEARCH SEARCH_COMMON
)

fast_downward_plugin(
    NAME PLUGIN_LAZY_WASTAR
    HELP "Weighted A* search with deferred evaluation (lazy)"
    SOURCES
        search_engines/plugin_lazy_wastar
    DEPENDS LAZY_SEARCH SEARCH_COMMON
)

fast_downward_plugin(
    NAME ENFORCED_HILL_CLIMBING_SEARCH
    HELP "Lazy enforced hill-climbing search algorithm"
    SOURCES
        search_engines/enforced_hill_climbing_search
    DEPENDS G_EVALUATOR ORDERED_SET PREF_EVALUATOR SEARCH_COMMON SUCCESSOR_GENERATOR
)

fast_downward_plugin(
    NAME ITERATED_SEARCH
    HELP "Iterated search algorithm"
    SOURCES
        search_engines/iterated_search
)

fast_downward_plugin(
    NAME LAZY_SEARCH
    HELP "Lazy search algorithm"
    SOURCES
        search_engines/lazy_search
    DEPENDS ORDERED_SET SUCCESSOR_GENERATOR
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME LP_SOLVER
    HELP "Interface to an LP solver"
    SOURCES
        lp/lp_internals
        lp/lp_solver
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME RELAXATION_HEURISTIC
    HELP "The base class for relaxation heuristics"
    SOURCES
        heuristics/relaxation_heuristic
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME ADDITIVE_HEURISTIC
    HELP "The additive heuristic"
    SOURCES
        heuristics/additive_heuristic
    DEPENDS PRIORITY_QUEUES RELAXATION_HEURISTIC TASK_PROPERTIES
)

fast_downward_plugin(
    NAME BLIND_SEARCH_HEURISTIC
    HELP "The 'blind search' heuristic"
    SOURCES
        heuristics/blind_search_heuristic
    DEPENDS TASK_PROPERTIES
)

fast_downward_plugin(
    NAME CONTEXT_ENHANCED_ADDITIVE_HEURISTIC
    HELP "The context-enhanced additive heuristic"
    SOURCES
        heuristics/cea_heuristic
    DEPENDS DOMAIN_TRANSITION_GRAPH PRIORITY_QUEUES TASK_PROPERTIES
)

fast_downward_plugin(
    NAME CG_HEURISTIC
    HELP "The causal graph heuristic"
    SOURCES heuristics/cg_heuristic
            heuristics/cg_cache
    DEPENDS DOMAIN_TRANSITION_GRAPH PRIORITY_QUEUES TASK_PROPERTIES
)

fast_downward_plugin(
    NAME DOMAIN_TRANSITION_GRAPH
    HELP "DTGs used by cg and cea heuristic"
    SOURCES
        heuristics/domain_transition_graph
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME FF_HEURISTIC
    HELP "The FF heuristic (an implementation of the RPG heuristic)"
    SOURCES
        heuristics/ff_heuristic
    DEPENDS ADDITIVE_HEURISTIC TASK_PROPERTIES
)

fast_downward_plugin(
    NAME GOAL_COUNT_HEURISTIC
    HELP "The goal-counting heuristic"
    SOURCES
        heuristics/goal_count_heuristic
)

fast_downward_plugin(
    NAME HM_HEURISTIC
    HELP "The h^m heuristic"
    SOURCES
        heuristics/hm_heuristic
    DEPENDS TASK_PROPERTIES
)

fast_downward_plugin(
    NAME LANDMARK_CUT_HEURISTIC
    HELP "The LM-cut heuristic"
    SOURCES
        heuristics/lm_cut_heuristic
        heuristics/lm_cut_landmarks
    DEPENDS PRIORITY_QUEUES TASK_PROPERTIES
)

fast_downward_plugin(
    NAME MAX_HEURISTIC
    HELP "The Max heuristic"
    SOURCES
        heuristics/max_heuristic
    DEPENDS PRIORITY_QUEUES RELAXATION_HEURISTIC
)

fast_downward_plugin(
    NAME CORE_TASKS
    HELP "Core task transformations"
    SOURCES
        tasks/cost_adapted_task
        tasks/delegating_task
        tasks/root_task
    CORE_PLUGIN
)

fast_downward_plugin(
    NAME EXTRA_TASKS
    HELP "Non-core task transformations"
    SOURCES
        tasks/domain_abstracted_task
        tasks/domain_abstracted_task_factory
        tasks/modified_goals_task
        tasks/modified_operator_costs_task
    DEPENDS TASK_PROPERTIES
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME CAUSAL_GRAPH
    HELP "Causal Graph"
    SOURCES
        task_utils/causal_graph
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME SAMPLING
    HELP "Sampling"
    SOURCES
        task_utils/sampling
    DEPENDS SUCCESSOR_GENERATOR TASK_PROPERTIES
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME SUCCESSOR_GENERATOR
    HELP "Successor generator"
    SOURCES
        task_utils/successor_generator
        task_utils/successor_generator_factory
        task_utils/successor_generator_internals
    DEPENDS TASK_PROPERTIES
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME TASK_PROPERTIES
    HELP "Task properties"
    SOURCES
        task_utils/task_properties
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME VARIABLE_ORDER_FINDER
    HELP "Variable order finder"
    SOURCES
        task_utils/variable_order_finder
    DEPENDENCY_ONLY
)

fast_downward_plugin(
    NAME CEGAR
    HELP "Plugin containing the code for CEGAR heuristics"
    SOURCES
        cegar/abstraction
        cegar/abstract_search
        cegar/abstract_state
        cegar/additive_cartesian_heuristic
        cegar/cartesian_heuristic_function
        cegar/cost_saturation
        cegar/domains
        cegar/refinement_hierarchy
        cegar/split_selector
        cegar/subtask_generators
        cegar/transition
        cegar/transition_updater
        cegar/utils
        cegar/utils_landmarks
    DEPENDS ADDITIVE_HEURISTIC DYNAMIC_BITSET EXTRA_TASKS LANDMARKS PRIORITY_QUEUES TASK_PROPERTIES
)

fast_downward_plugin(
    NAME MAS_HEURISTIC
    HELP "The Merge-and-Shrink heuristic"
    SOURCES
        merge_and_shrink/distances
        merge_and_shrink/factored_transition_system
        merge_and_shrink/fts_factory
        merge_and_shrink/label_equivalence_relation
        merge_and_shrink/label_reduction
        merge_and_shrink/labels
        merge_and_shrink/merge_and_shrink_heuristic
        merge_and_shrink/merge_and_shrink_representation
        merge_and_shrink/merge_scoring_function
        merge_and_shrink/merge_scoring_function_dfp
        merge_and_shrink/merge_scoring_function_goal_relevance
        merge_and_shrink/merge_scoring_function_single_random
        merge_and_shrink/merge_scoring_function_total_order
        merge_and_shrink/merge_selector
        merge_and_shrink/merge_selector_score_based_filtering
        merge_and_shrink/merge_strategy
        merge_and_shrink/merge_strategy_aliases
        merge_and_shrink/merge_strategy_factory
        merge_and_shrink/merge_strategy_factory_precomputed
        merge_and_shrink/merge_strategy_factory_sccs
        merge_and_shrink/merge_strategy_factory_stateless
        merge_and_shrink/merge_strategy_precomputed
        merge_and_shrink/merge_strategy_sccs
        merge_and_shrink/merge_strategy_stateless
        merge_and_shrink/merge_tree
        merge_and_shrink/merge_tree_factory
        merge_and_shrink/merge_tree_factory_linear
        merge_and_shrink/shrink_bisimulation
        merge_and_shrink/shrink_bucket_based
        merge_and_shrink/shrink_fh
        merge_and_shrink/shrink_random
        merge_and_shrink/shrink_strategy
        merge_and_shrink/transition_system
        merge_and_shrink/types
        merge_and_shrink/utils
    DEPENDS PRIORITY_QUEUES EQUIVALENCE_RELATION SCCS TASK_PROPERTIES VARIABLE_ORDER_FINDER
)

fast_downward_plugin(
    NAME LANDMARKS
    HELP "Plugin containing the code to reason with landmarks"
    SOURCES
        landmarks/exploration
        landmarks/lama_ff_synergy
        landmarks/landmark_cost_assignment
        landmarks/landmark_count_heuristic
        landmarks/landmark_factory
        landmarks/landmark_factory_h_m
        landmarks/landmark_factory_merged
        landmarks/landmark_factory_rpg_exhaust
        landmarks/landmark_factory_rpg_sasp
        landmarks/landmark_factory_zhu_givan
        landmarks/landmark_graph
        landmarks/landmark_status_manager
        landmarks/util
    DEPENDS LP_SOLVER PRIORITY_QUEUES SUCCESSOR_GENERATOR TASK_PROPERTIES
)

fast_downward_plugin(
    NAME OPERATOR_COUNTING
    HELP "Plugin containing the code for operator counting heuristics"
    SOURCES
        operator_counting/constraint_generator
        operator_counting/lm_cut_constraints
        operator_counting/operator_counting_heuristic
        operator_counting/pho_constraints
        operator_counting/state_equation_constraints
    DEPENDS LP_SOLVER LANDMARK_CUT_HEURISTIC PDBS TASK_PROPERTIES
)

fast_downward_plugin(
    NAME PDBS
    HELP "Plugin containing the code for PDBs"
    SOURCES
        pdbs/canonical_pdbs
        pdbs/canonical_pdbs_heuristic
        pdbs/dominance_pruning
        pdbs/incremental_canonical_pdbs
        pdbs/match_tree
        pdbs/max_additive_pdb_sets
        pdbs/max_cliques
        pdbs/pattern_collection_information
        pdbs/pattern_database
        pdbs/pattern_collection_generator_combo
        pdbs/pattern_collection_generator_genetic
        pdbs/pattern_collection_generator_hillclimbing
        pdbs/pattern_collection_generator_manual
        pdbs/pattern_collection_generator_systematic
        pdbs/pattern_generator_greedy
        pdbs/pattern_generator_manual
        pdbs/pattern_generator
        pdbs/pdb_heuristic
        pdbs/types
        pdbs/validation
        pdbs/zero_one_pdbs
        pdbs/zero_one_pdbs_heuristic
    DEPENDS CAUSAL_GRAPH MAX_CLIQUES PRIORITY_QUEUES SAMPLING SUCCESSOR_GENERATOR TASK_PROPERTIES VARIABLE_ORDER_FINDER
)

fast_downward_plugin(
    NAME POTENTIALS
    HELP "Plugin containing the code for potential heuristics"
    SOURCES
        potentials/diverse_potential_heuristics
        potentials/potential_function
        potentials/potential_heuristic
        potentials/potential_max_heuristic
        potentials/potential_optimizer
        potentials/sample_based_potential_heuristics
        potentials/single_potential_heuristics
        potentials/util
    DEPENDS LP_SOLVER SAMPLING SUCCESSOR_GENERATOR TASK_PROPERTIES
)

fast_downward_plugin(
    NAME SCCS
    HELP "Algorithm to compute the strongly connected components (SCCs) of a "
         "directed graph."
    SOURCES
        algorithms/sccs.cc
    DEPENDENCY_ONLY
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
