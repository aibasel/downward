# See https://www.fast-downward.org/ForDevelopers/AddingSourceFiles
# for general information on adding source files and CMake libraries.
#
# All libraries are enabled by default and users can disable them by specifying
#    -DLIBRARY_FOO_ENABLED=FALSE
# The default behavior can be changed so all non-essential libraries are
# disabled by default by specifying
#    -DDISABLE_LIBRARIES_BY_DEFAULT=TRUE
# In that case, individual libraries can be enabled with
#    -DLIBRARY_FOO_ENABLED=TRUE
#
# Defining a new library:
#    create_fast_downward_library(
#        NAME <NAME>
#        [ DISPLAY_NAME <DISPLAY_NAME> ]
#        [ HELP <HELP> ]
#        SOURCES
#            <FILE_1> [ <FILE_2> ... ]
#        [ DEPENDS <LIBRARY_NAME_1> [ <LIBRARY_NAME_2> ... ] ]
#        [ DEPENDENCY_ONLY ]
#        [ CORE_LIBRARY ]
#    )
#
# <DISPLAY_NAME> defaults to lower case <NAME> and is used to group files
#   in IDEs and for messages.
# <HELP> defaults to <DISPLAY_NAME> and is used to describe the cmake option.
# SOURCES lists the source files that are part of the library. Entries are
#   listed without extension. For an entry <file>, both <file>.h and <file>.cc
#   are added if the files exist.
# DEPENDS lists libraries that will be automatically enabled if this library is
#   enabled. If the dependency was not enabled before, this will be logged.
# DEPENDENCY_ONLY disables the library unless it is needed as a dependency and
#   hides the option to enable the library in cmake GUIs like ccmake.
# CORE_LIBRARY always enables the library (even if DISABLE_LIBRARIES_BY_DEFAULT
#   is used) and hides the option to disable it in CMake GUIs like ccmake.

cmake_minimum_required(VERSION 3.16)

# Path containing custom CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(macros)

report_bitwidth()
set_up_build_types("Debug;Release")
set_up_options()

project(downward LANGUAGES CXX)
add_executable(downward planner.cc)

# On Windows we have to copy all DLLs next to the generated binary.
copy_dlls_to_binary_dir(downward)

create_fast_downward_library(
    NAME CORE_SOURCES
    HELP "Core source files"
    SOURCES
        abstract_task
        axioms
        command_line
        evaluation_context
        evaluation_result
        evaluator
        evaluator_cache
        heuristic
        open_list
        open_list_factory
        operator_cost
        operator_id
        per_state_array
        per_state_bitset
        per_state_information
        per_task_information
        plan_manager
        pruning_method
        search_algorithm
        search_node_info
        search_progress
        search_space
        search_statistics
        state_id
        state_registry
        task_id
        task_proxy

    DEPENDS CAUSAL_GRAPH INT_HASH_SET INT_PACKER ORDERED_SET SEGMENTED_VECTOR SUBSCRIBER SUCCESSOR_GENERATOR TASK_PROPERTIES
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME PLUGINS
    HELP "Plugin definition"
    SOURCES
        plugins/any
        plugins/bounds
        plugins/doc_printer
        plugins/options
        plugins/plugin
        plugins/plugin_info
        plugins/raw_registry
        plugins/registry
        plugins/registry_types
        plugins/types
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME PARSER
    HELP "Option parsing"
    SOURCES
        parser/abstract_syntax_tree
        parser/decorated_abstract_syntax_tree
        parser/lexical_analyzer
        parser/syntax_analyzer
        parser/token_stream
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME UTILS
    HELP "System utilities"
    SOURCES
        utils/collections
        utils/countdown_timer
        utils/exceptions
        utils/hash
        utils/language
        utils/logging
        utils/markup
        utils/math
        utils/memory
        utils/rng
        utils/rng_options
        utils/strings
        utils/system
        utils/system_unix
        utils/system_windows
        utils/timer
    CORE_LIBRARY
)
# On Linux, find the rt library for clock_gettime().
if(UNIX AND NOT APPLE)
    target_link_libraries(downward_UTILS INTERFACE rt)
endif()
# On Windows, find the psapi library for determining peak memory.
if(WIN32)
    cmake_policy(SET CMP0074 NEW)
    target_link_libraries(downward_UTILS INTERFACE psapi)
endif()


create_fast_downward_library(
    NAME ALTERNATION_OPEN_LIST
    HELP "Open list that alternates between underlying open lists in a round-robin manner"
    SOURCES
        open_lists/alternation_open_list
)

create_fast_downward_library(
    NAME BEST_FIRST_OPEN_LIST
    HELP "Open list that selects the best element according to a single evaluation function"
    SOURCES
        open_lists/best_first_open_list
)

create_fast_downward_library(
    NAME EPSILON_GREEDY_OPEN_LIST
    HELP "Open list that chooses an entry randomly with probability epsilon"
    SOURCES
        open_lists/epsilon_greedy_open_list
)

create_fast_downward_library(
    NAME PARETO_OPEN_LIST
    HELP "Pareto open list"
    SOURCES
        open_lists/pareto_open_list
)

create_fast_downward_library(
    NAME TIEBREAKING_OPEN_LIST
    HELP "Tiebreaking open list"
    SOURCES
        open_lists/tiebreaking_open_list
)

create_fast_downward_library(
    NAME TYPE_BASED_OPEN_LIST
    HELP "Type-based open list"
    SOURCES
        open_lists/type_based_open_list
)

create_fast_downward_library(
    NAME DYNAMIC_BITSET
    HELP "Poor man's version of boost::dynamic_bitset"
    SOURCES
        algorithms/dynamic_bitset
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME NAMED_VECTOR
    HELP "Generic vector with associated name for each element"
    SOURCES
        algorithms/named_vector
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME EQUIVALENCE_RELATION
    HELP "Equivalence relation over [1, ..., n] that can be iteratively refined"
    SOURCES
        algorithms/equivalence_relation
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME INT_HASH_SET
    HELP "Hash set storing non-negative integers"
    SOURCES
        algorithms/int_hash_set
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME INT_PACKER
    HELP "Greedy bin packing algorithm to pack integer variables with small domains tightly into memory"
    SOURCES
        algorithms/int_packer
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME MAX_CLIQUES
    HELP "Implementation of the Max Cliques algorithm by Tomita et al."
    SOURCES
        algorithms/max_cliques
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME PRIORITY_QUEUES
    HELP "Three implementations of priority queue: HeapQueue, BucketQueue and AdaptiveQueue"
    SOURCES
        algorithms/priority_queues
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME ORDERED_SET
    HELP "Set of elements ordered by insertion time"
    SOURCES
        algorithms/ordered_set
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME SEGMENTED_VECTOR
    HELP "Memory-friendly and vector-like data structure"
    SOURCES
        algorithms/segmented_vector
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME SUBSCRIBER
    HELP "Allows object to subscribe to the destructor of other objects"
    SOURCES
        algorithms/subscriber
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME EVALUATORS_SUBCATEGORY
    HELP "Subcategory plugin for basic evaluators"
    SOURCES
        evaluators/subcategory
)

create_fast_downward_library(
    NAME CONST_EVALUATOR
    HELP "The constant evaluator"
    SOURCES
        evaluators/const_evaluator
    DEPENDS EVALUATORS_SUBCATEGORY
)

create_fast_downward_library(
    NAME G_EVALUATOR
    HELP "The g-evaluator"
    SOURCES
        evaluators/g_evaluator
    DEPENDS EVALUATORS_SUBCATEGORY
)

create_fast_downward_library(
    NAME COMBINING_EVALUATOR
    HELP "The combining evaluator"
    SOURCES
        evaluators/combining_evaluator
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME MAX_EVALUATOR
    HELP "The max evaluator"
    SOURCES
        evaluators/max_evaluator
    DEPENDS COMBINING_EVALUATOR EVALUATORS_SUBCATEGORY
)

create_fast_downward_library(
    NAME PREF_EVALUATOR
    HELP "The pref evaluator"
    SOURCES
        evaluators/pref_evaluator
    DEPENDS EVALUATORS_SUBCATEGORY
)

create_fast_downward_library(
    NAME WEIGHTED_EVALUATOR
    HELP "The weighted evaluator"
    SOURCES
        evaluators/weighted_evaluator
    DEPENDS EVALUATORS_SUBCATEGORY
)

create_fast_downward_library(
    NAME SUM_EVALUATOR
    HELP "The sum evaluator"
    SOURCES
        evaluators/sum_evaluator
    DEPENDS COMBINING_EVALUATOR EVALUATORS_SUBCATEGORY
)

create_fast_downward_library(
    NAME NULL_PRUNING_METHOD
    HELP "Pruning method that does nothing"
    SOURCES
        pruning/null_pruning_method
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME LIMITED_PRUNING
    HELP "Method for limiting another pruning method"
    SOURCES
        pruning/limited_pruning
)

create_fast_downward_library(
    NAME STUBBORN_SETS
    HELP "Base class for all stubborn set partial order reduction methods"
    SOURCES
        pruning/stubborn_sets
    DEPENDS TASK_PROPERTIES
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME STUBBORN_SETS_ACTION_CENTRIC
    HELP "Base class for all action-centric stubborn set partial order reduction methods"
    SOURCES
        pruning/stubborn_sets_action_centric
    DEPENDS STUBBORN_SETS
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME STUBBORN_SETS_ATOM_CENTRIC
    HELP "Atom-centric stubborn sets"
    SOURCES
        pruning/stubborn_sets_atom_centric
    DEPENDS STUBBORN_SETS
)

create_fast_downward_library(
    NAME STUBBORN_SETS_SIMPLE
    HELP "Stubborn sets simple"
    SOURCES
        pruning/stubborn_sets_simple
    DEPENDS STUBBORN_SETS_ACTION_CENTRIC
)

create_fast_downward_library(
    NAME STUBBORN_SETS_EC
    HELP "Stubborn set method that dominates expansion core"
    SOURCES
        pruning/stubborn_sets_ec
    DEPENDS STUBBORN_SETS_ACTION_CENTRIC TASK_PROPERTIES
)

create_fast_downward_library(
    NAME SEARCH_COMMON
    HELP "Basic classes used for all search algorithms"
    SOURCES
        search_algorithms/search_common
    DEPENDS ALTERNATION_OPEN_LIST G_EVALUATOR BEST_FIRST_OPEN_LIST SUM_EVALUATOR TIEBREAKING_OPEN_LIST WEIGHTED_EVALUATOR
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME EAGER_SEARCH
    HELP "Eager search"
    SOURCES
        search_algorithms/eager_search
    DEPENDS NULL_PRUNING_METHOD ORDERED_SET SUCCESSOR_GENERATOR
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME PLUGIN_ASTAR
    HELP "A* search"
    SOURCES
        search_algorithms/plugin_astar
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME PLUGIN_EAGER
    HELP "Eager (i.e., normal) best-first search"
    SOURCES
        search_algorithms/plugin_eager
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME PLUGIN_EAGER_GREEDY
    HELP "Eager greedy best-first search"
    SOURCES
        search_algorithms/plugin_eager_greedy
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME PLUGIN_EAGER_WASTAR
    HELP "Weighted eager A* search"
    SOURCES
        search_algorithms/plugin_eager_wastar
    DEPENDS EAGER_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME PLUGIN_LAZY
    HELP "Best-first search with deferred evaluation (lazy)"
    SOURCES
        search_algorithms/plugin_lazy
    DEPENDS LAZY_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME PLUGIN_LAZY_GREEDY
    HELP "Greedy best-first search with deferred evaluation (lazy)"
    SOURCES
        search_algorithms/plugin_lazy_greedy
    DEPENDS LAZY_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME PLUGIN_LAZY_WASTAR
    HELP "Weighted A* search with deferred evaluation (lazy)"
    SOURCES
        search_algorithms/plugin_lazy_wastar
    DEPENDS LAZY_SEARCH SEARCH_COMMON
)

create_fast_downward_library(
    NAME ENFORCED_HILL_CLIMBING_SEARCH
    HELP "Lazy enforced hill-climbing search"
    SOURCES
        search_algorithms/enforced_hill_climbing_search
    DEPENDS G_EVALUATOR ORDERED_SET PREF_EVALUATOR SEARCH_COMMON SUCCESSOR_GENERATOR
)

create_fast_downward_library(
    NAME ITERATED_SEARCH
    HELP "Iterated search"
    SOURCES
        search_algorithms/iterated_search
)

create_fast_downward_library(
    NAME LAZY_SEARCH
    HELP "Lazy search"
    SOURCES
        search_algorithms/lazy_search
    DEPENDS ORDERED_SET SUCCESSOR_GENERATOR
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME LP_SOLVER
    HELP "Interface to an LP solver"
    SOURCES
        lp/lp_internals
        lp/lp_solver
        lp/solver_interface
    DEPENDS NAMED_VECTOR
    DEPENDENCY_ONLY
)
if(USE_LP)
    find_package(Cplex 12)
    if(CPLEX_FOUND)
        add_library(downward_cplex_interface INTERFACE)
        target_compile_definitions(downward_cplex_interface INTERFACE HAS_CPLEX)
        target_link_libraries(downward_cplex_interface INTERFACE cplex::cplex)
        target_sources(downward_cplex_interface INTERFACE lp/cplex_solver_interface.h lp/cplex_solver_interface.cc)

        target_link_libraries(downward_LP_SOLVER INTERFACE downward_cplex_interface)
    endif()

    # TODO: we actually require a version greater than 6.0.3 but it is not released yet.
    find_package(soplex 6.0.3 QUIET)
    if (SOPLEX_FOUND)
        message(STATUS "Found SoPlex: ${SOPLEX_INCLUDE_DIRS}")
        add_library(downward_soplex_interface INTERFACE)
        target_link_libraries(downward_soplex_interface INTERFACE libsoplex)
        target_compile_definitions(downward_soplex_interface INTERFACE HAS_SOPLEX)
        target_sources(downward_soplex_interface INTERFACE lp/soplex_solver_interface.h lp/soplex_solver_interface.cc)

        target_link_libraries(downward_LP_SOLVER INTERFACE downward_soplex_interface)
    endif()
endif()

create_fast_downward_library(
    NAME RELAXATION_HEURISTIC
    HELP "The base class for relaxation heuristics"
    SOURCES
        heuristics/array_pool
        heuristics/relaxation_heuristic
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME ADDITIVE_HEURISTIC
    HELP "The additive heuristic"
    SOURCES
        heuristics/additive_heuristic
    DEPENDS PRIORITY_QUEUES RELAXATION_HEURISTIC TASK_PROPERTIES
)

create_fast_downward_library(
    NAME BLIND_SEARCH_HEURISTIC
    HELP "The 'blind search' heuristic"
    SOURCES
        heuristics/blind_search_heuristic
    DEPENDS TASK_PROPERTIES
)

create_fast_downward_library(
    NAME CONTEXT_ENHANCED_ADDITIVE_HEURISTIC
    HELP "The context-enhanced additive heuristic"
    SOURCES
        heuristics/cea_heuristic
    DEPENDS DOMAIN_TRANSITION_GRAPH PRIORITY_QUEUES TASK_PROPERTIES
)

create_fast_downward_library(
    NAME CG_HEURISTIC
    HELP "The causal graph heuristic"
    SOURCES heuristics/cg_heuristic
            heuristics/cg_cache
    DEPENDS DOMAIN_TRANSITION_GRAPH PRIORITY_QUEUES TASK_PROPERTIES
)

create_fast_downward_library(
    NAME DOMAIN_TRANSITION_GRAPH
    HELP "DTGs used by cg and cea heuristic"
    SOURCES
        heuristics/domain_transition_graph
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME FF_HEURISTIC
    HELP "The FF heuristic (an implementation of the RPG heuristic)"
    SOURCES
        heuristics/ff_heuristic
    DEPENDS ADDITIVE_HEURISTIC TASK_PROPERTIES
)

create_fast_downward_library(
    NAME GOAL_COUNT_HEURISTIC
    HELP "The goal-counting heuristic"
    SOURCES
        heuristics/goal_count_heuristic
)

create_fast_downward_library(
    NAME HM_HEURISTIC
    HELP "The h^m heuristic"
    SOURCES
        heuristics/hm_heuristic
    DEPENDS TASK_PROPERTIES
)

create_fast_downward_library(
    NAME LANDMARK_CUT_HEURISTIC
    HELP "The LM-cut heuristic"
    SOURCES
        heuristics/lm_cut_heuristic
        heuristics/lm_cut_landmarks
    DEPENDS PRIORITY_QUEUES TASK_PROPERTIES
)

create_fast_downward_library(
    NAME MAX_HEURISTIC
    HELP "The Max heuristic"
    SOURCES
        heuristics/max_heuristic
    DEPENDS PRIORITY_QUEUES RELAXATION_HEURISTIC
)

create_fast_downward_library(
    NAME CORE_TASKS
    HELP "Core task transformations"
    SOURCES
        tasks/cost_adapted_task
        tasks/delegating_task
        tasks/root_task
    CORE_LIBRARY
)

create_fast_downward_library(
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

create_fast_downward_library(
    NAME CAUSAL_GRAPH
    HELP "Causal Graph"
    SOURCES
        task_utils/causal_graph
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME SAMPLING
    HELP "Sampling"
    SOURCES
        task_utils/sampling
    DEPENDS SUCCESSOR_GENERATOR TASK_PROPERTIES
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME SUCCESSOR_GENERATOR
    HELP "Successor generator"
    SOURCES
        task_utils/successor_generator
        task_utils/successor_generator_factory
        task_utils/successor_generator_internals
    DEPENDS TASK_PROPERTIES
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME TASK_PROPERTIES
    HELP "Task properties"
    SOURCES
        task_utils/task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME VARIABLE_ORDER_FINDER
    HELP "Variable order finder"
    SOURCES
        task_utils/variable_order_finder
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME CEGAR
    HELP "Plugin containing the code for Cartesian CEGAR heuristics"
    SOURCES
        cartesian_abstractions/abstraction
        cartesian_abstractions/abstract_search
        cartesian_abstractions/abstract_state
        cartesian_abstractions/additive_cartesian_heuristic
        cartesian_abstractions/cartesian_heuristic_function
        cartesian_abstractions/cartesian_set
        cartesian_abstractions/cegar
        cartesian_abstractions/cost_saturation
        cartesian_abstractions/refinement_hierarchy
        cartesian_abstractions/split_selector
        cartesian_abstractions/subtask_generators
        cartesian_abstractions/transition
        cartesian_abstractions/transition_system
        cartesian_abstractions/types
        cartesian_abstractions/utils
        cartesian_abstractions/utils_landmarks
    DEPENDS ADDITIVE_HEURISTIC DYNAMIC_BITSET EXTRA_TASKS LANDMARKS PRIORITY_QUEUES TASK_PROPERTIES
)

create_fast_downward_library(
    NAME MAS_HEURISTIC
    HELP "The Merge-and-Shrink heuristic"
    SOURCES
        merge_and_shrink/distances
        merge_and_shrink/factored_transition_system
        merge_and_shrink/fts_factory
        merge_and_shrink/label_reduction
        merge_and_shrink/labels
        merge_and_shrink/merge_and_shrink_algorithm
        merge_and_shrink/merge_and_shrink_heuristic
        merge_and_shrink/merge_and_shrink_representation
        merge_and_shrink/merge_scoring_function
        merge_and_shrink/merge_scoring_function_dfp
        merge_and_shrink/merge_scoring_function_goal_relevance
        merge_and_shrink/merge_scoring_function_miasm
        merge_and_shrink/merge_scoring_function_miasm_utils
        merge_and_shrink/merge_scoring_function_single_random
        merge_and_shrink/merge_scoring_function_total_order
        merge_and_shrink/merge_selector
        merge_and_shrink/merge_selector_score_based_filtering
        merge_and_shrink/merge_strategy
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

create_fast_downward_library(
    NAME LANDMARKS
    HELP "Plugin containing the code to reason with landmarks"
    SOURCES
        landmarks/exploration
        landmarks/landmark
        landmarks/landmark_cost_assignment
        landmarks/landmark_cost_partitioning_heuristic
        landmarks/landmark_factory
        landmarks/landmark_factory_h_m
        landmarks/landmark_factory_reasonable_orders_hps
        landmarks/landmark_factory_merged
        landmarks/landmark_factory_relaxation
        landmarks/landmark_factory_rpg_exhaust
        landmarks/landmark_factory_rpg_sasp
        landmarks/landmark_factory_zhu_givan
        landmarks/landmark_graph
        landmarks/landmark_heuristic
        landmarks/landmark_status_manager
        landmarks/landmark_sum_heuristic
        landmarks/util
    DEPENDS LP_SOLVER PRIORITY_QUEUES SUCCESSOR_GENERATOR TASK_PROPERTIES
)

create_fast_downward_library(
    NAME OPERATOR_COUNTING
    HELP "Plugin containing the code for operator-counting heuristics"
    SOURCES
        operator_counting/constraint_generator
        operator_counting/delete_relaxation_constraints
        operator_counting/lm_cut_constraints
        operator_counting/operator_counting_heuristic
        operator_counting/pho_constraints
        operator_counting/state_equation_constraints
    DEPENDS LP_SOLVER LANDMARK_CUT_HEURISTIC PDBS TASK_PROPERTIES
)

create_fast_downward_library(
    NAME PDBS
    HELP "Plugin containing the code for PDBs"
    SOURCES
        pdbs/abstract_operator
        pdbs/canonical_pdbs
        pdbs/canonical_pdbs_heuristic
        pdbs/cegar
        pdbs/dominance_pruning
        pdbs/incremental_canonical_pdbs
        pdbs/match_tree
        pdbs/max_cliques
        pdbs/pattern_cliques
        pdbs/pattern_collection_information
        pdbs/pattern_collection_generator_combo
        pdbs/pattern_collection_generator_disjoint_cegar
        pdbs/pattern_collection_generator_genetic
        pdbs/pattern_collection_generator_hillclimbing
        pdbs/pattern_collection_generator_manual
        pdbs/pattern_collection_generator_multiple_cegar
        pdbs/pattern_collection_generator_multiple_random
        pdbs/pattern_collection_generator_multiple
        pdbs/pattern_collection_generator_systematic
        pdbs/pattern_database_factory
        pdbs/pattern_database
        pdbs/pattern_generator_cegar
        pdbs/pattern_generator_greedy
        pdbs/pattern_generator_manual
        pdbs/pattern_generator_random
        pdbs/pattern_generator
        pdbs/pattern_information
        pdbs/pdb_heuristic
        pdbs/random_pattern
        pdbs/subcategory
        pdbs/types
        pdbs/utils
        pdbs/validation
        pdbs/zero_one_pdbs
        pdbs/zero_one_pdbs_heuristic
    DEPENDS CAUSAL_GRAPH MAX_CLIQUES PRIORITY_QUEUES SAMPLING SUCCESSOR_GENERATOR TASK_PROPERTIES VARIABLE_ORDER_FINDER
)

create_fast_downward_library(
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
        potentials/subcategory
        potentials/util
    DEPENDS LP_SOLVER SAMPLING SUCCESSOR_GENERATOR TASK_PROPERTIES
)

create_fast_downward_library(
    NAME SCCS
    HELP "Algorithm to compute the strongly connected components (SCCs) of a "
         "directed graph."
    SOURCES
        algorithms/sccs
    DEPENDENCY_ONLY
)
