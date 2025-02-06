# See https://www.fast-downward.org/ForDevelopers/CMake
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
#        [ HELP <HELP> ]
#        SOURCES
#            <FILE_1> [ <FILE_2> ... ]
#        [ DEPENDS <LIBRARY_NAME_1> [ <LIBRARY_NAME_2> ... ] ]
#        [ DEPENDENCY_ONLY ]
#        [ CORE_LIBRARY ]
#    )
#
# <HELP> is used to describe the cmake option, for example in ccmake.
# SOURCES lists the source files that are part of the library. Entries are
#    listed without extension. For an entry <file>, both <file>.h and <file>.cc
#    are added if the files exist.
# DEPENDS lists libraries that will be compiled as dependendies if this library
#    is enabled.
# DEPENDENCY_ONLY disables the library unless it is needed as a dependency and
#    hides the option to enable the library in cmake GUIs like ccmake.
# CORE_LIBRARY always enables the library (even if DISABLE_LIBRARIES_BY_DEFAULT
#    is used) and hides the option to disable it in CMake GUIs like ccmake.

cmake_minimum_required(VERSION 3.16)

# Path containing custom CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(common_cxx_flags)
include(macros)
include(options)

report_bitwidth()
set_up_build_types("Debug;Release")
set_up_options()

project(downward LANGUAGES CXX)
add_executable(downward planner.cc)

# On Windows we have to copy all DLLs next to the generated binary.
if (WIN32)
    copy_dlls_to_binary_dir_after_build(downward)
endif()

# In the following, we include all source files, grouped into libraries with
# dependencies among each other.

create_fast_downward_library(
    NAME core_sources
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
    DEPENDS
        causal_graph
        int_hash_set
        int_packer
        ordered_set
        segmented_vector
        subscriber
        successor_generator
        task_properties
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME plugins
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
    NAME parser
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
    NAME utils
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
        utils/tuples
    CORE_LIBRARY
)
# On Linux, find the rt library for clock_gettime().
if(UNIX AND NOT APPLE)
    target_link_libraries(utils INTERFACE rt)
endif()
# On Windows, find the psapi library for determining peak memory.
if(WIN32)
    cmake_policy(SET CMP0074 NEW)
    target_link_libraries(utils INTERFACE psapi)
endif()

create_fast_downward_library(
    NAME alternation_open_list
    HELP "Open list that alternates between underlying open lists in a round-robin manner"
    SOURCES
        open_lists/alternation_open_list
)

create_fast_downward_library(
    NAME best_first_open_list
    HELP "Open list that selects the best element according to a single evaluation function"
    SOURCES
        open_lists/best_first_open_list
)

create_fast_downward_library(
    NAME epsilon_greedy_open_list
    HELP "Open list that chooses an entry randomly with probability epsilon"
    SOURCES
        open_lists/epsilon_greedy_open_list
)

create_fast_downward_library(
    NAME pareto_open_list
    HELP "Pareto open list"
    SOURCES
        open_lists/pareto_open_list
)

create_fast_downward_library(
    NAME tiebreaking_open_list
    HELP "Tiebreaking open list"
    SOURCES
        open_lists/tiebreaking_open_list
)

create_fast_downward_library(
    NAME type_based_open_list
    HELP "Type-based open list"
    SOURCES
        open_lists/type_based_open_list
)

create_fast_downward_library(
    NAME dynamic_bitset
    HELP "Poor man's version of boost::dynamic_bitset"
    SOURCES
        algorithms/dynamic_bitset
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME named_vector
    HELP "Generic vector with associated name for each element"
    SOURCES
        algorithms/named_vector
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME equivalence_relation
    HELP "Equivalence relation over [1, ..., n] that can be iteratively refined"
    SOURCES
        algorithms/equivalence_relation
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME int_hash_set
    HELP "Hash set storing non-negative integers"
    SOURCES
        algorithms/int_hash_set
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME int_packer
    HELP "Greedy bin packing algorithm to pack integer variables with small domains tightly into memory"
    SOURCES
        algorithms/int_packer
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME max_cliques
    HELP "Implementation of the Max Cliques algorithm by Tomita et al."
    SOURCES
        algorithms/max_cliques
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME priority_queues
    HELP "Three implementations of priority queue: HeapQueue, BucketQueue and AdaptiveQueue"
    SOURCES
        algorithms/priority_queues
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME ordered_set
    HELP "Set of elements ordered by insertion time"
    SOURCES
        algorithms/ordered_set
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME segmented_vector
    HELP "Memory-friendly and vector-like data structure"
    SOURCES
        algorithms/segmented_vector
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME subscriber
    HELP "Allows object to subscribe to the destructor of other objects"
    SOURCES
        algorithms/subscriber
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME evaluators_subcategory
    HELP "Subcategory plugin for basic evaluators"
    SOURCES
        evaluators/subcategory
)

create_fast_downward_library(
    NAME const_evaluator
    HELP "The constant evaluator"
    SOURCES
        evaluators/const_evaluator
    DEPENDS
        evaluators_subcategory
)

create_fast_downward_library(
    NAME g_evaluator
    HELP "The g-evaluator"
    SOURCES
        evaluators/g_evaluator
    DEPENDS evaluators_subcategory
)

create_fast_downward_library(
    NAME combining_evaluator
    HELP "The combining evaluator"
    SOURCES
        evaluators/combining_evaluator
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME max_evaluator
    HELP "The max evaluator"
    SOURCES
        evaluators/max_evaluator
    DEPENDS
        combining_evaluator
        evaluators_subcategory
)

create_fast_downward_library(
    NAME pref_evaluator
    HELP "The pref evaluator"
    SOURCES
        evaluators/pref_evaluator
    DEPENDS
        evaluators_subcategory
)

create_fast_downward_library(
    NAME weighted_evaluator
    HELP "The weighted evaluator"
    SOURCES
        evaluators/weighted_evaluator
    DEPENDS
        evaluators_subcategory
)

create_fast_downward_library(
    NAME sum_evaluator
    HELP "The sum evaluator"
    SOURCES
        evaluators/sum_evaluator
    DEPENDS
        combining_evaluator
        evaluators_subcategory
)

create_fast_downward_library(
    NAME null_pruning_method
    HELP "Pruning method that does nothing"
    SOURCES
        pruning/null_pruning_method
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME limited_pruning
    HELP "Method for limiting another pruning method"
    SOURCES
        pruning/limited_pruning
)

create_fast_downward_library(
    NAME stubborn_sets
    HELP "Base class for all stubborn set partial order reduction methods"
    SOURCES
        pruning/stubborn_sets
    DEPENDS
        task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME stubborn_sets_action_centric
    HELP "Base class for all action-centric stubborn set partial order reduction methods"
    SOURCES
        pruning/stubborn_sets_action_centric
    DEPENDS
        stubborn_sets
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME stubborn_sets_atom_centric
    HELP "Atom-centric stubborn sets"
    SOURCES
        pruning/stubborn_sets_atom_centric
    DEPENDS
        stubborn_sets
)

create_fast_downward_library(
    NAME stubborn_sets_simple
    HELP "Stubborn sets simple"
    SOURCES
        pruning/stubborn_sets_simple
    DEPENDS
        stubborn_sets_action_centric
)

create_fast_downward_library(
    NAME stubborn_sets_ec
    HELP "Stubborn set method that dominates expansion core"
    SOURCES
        pruning/stubborn_sets_ec
    DEPENDS
        stubborn_sets_action_centric
        task_properties
)

create_fast_downward_library(
    NAME search_common
    HELP "Basic classes used for all search algorithms"
    SOURCES
        search_algorithms/search_common
    DEPENDS
        alternation_open_list
        g_evaluator
        best_first_open_list
        sum_evaluator
        tiebreaking_open_list
        weighted_evaluator
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME eager_search
    HELP "Eager search"
    SOURCES
        search_algorithms/eager_search
    DEPENDS
        null_pruning_method
        ordered_set
        successor_generator
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME plugin_astar
    HELP "A* search"
    SOURCES
        search_algorithms/plugin_astar
    DEPENDS
        eager_search
        search_common
)

create_fast_downward_library(
    NAME plugin_eager
    HELP "Eager (i.e., normal) best-first search"
    SOURCES
        search_algorithms/plugin_eager
    DEPENDS
        eager_search
        search_common
)

create_fast_downward_library(
    NAME plugin_eager_greedy
    HELP "Eager greedy best-first search"
    SOURCES
        search_algorithms/plugin_eager_greedy
    DEPENDS
        eager_search
        search_common
)

create_fast_downward_library(
    NAME plugin_eager_wastar
    HELP "Weighted eager A* search"
    SOURCES
        search_algorithms/plugin_eager_wastar
    DEPENDS
        eager_search
        search_common
)

create_fast_downward_library(
    NAME plugin_lazy
    HELP "Best-first search with deferred evaluation (lazy)"
    SOURCES
        search_algorithms/plugin_lazy
    DEPENDS
        lazy_search
        search_common
)

create_fast_downward_library(
    NAME plugin_lazy_greedy
    HELP "Greedy best-first search with deferred evaluation (lazy)"
    SOURCES
        search_algorithms/plugin_lazy_greedy
    DEPENDS
        lazy_search
        search_common
)

create_fast_downward_library(
    NAME plugin_lazy_wastar
    HELP "Weighted A* search with deferred evaluation (lazy)"
    SOURCES
        search_algorithms/plugin_lazy_wastar
    DEPENDS
        lazy_search
        search_common
)

create_fast_downward_library(
    NAME enforced_hill_climbing_search
    HELP "Lazy enforced hill-climbing search"
    SOURCES
        search_algorithms/enforced_hill_climbing_search
    DEPENDS
        g_evaluator
        ordered_set
        pref_evaluator
        search_common
        successor_generator
)

create_fast_downward_library(
    NAME iterated_search
    HELP "Iterated search"
    SOURCES
        search_algorithms/iterated_search
)

create_fast_downward_library(
    NAME lazy_search
    HELP "Lazy search"
    SOURCES
        search_algorithms/lazy_search
    DEPENDS
        ordered_set
        successor_generator
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME lp_solver
    HELP "Interface to an LP solver"
    SOURCES
        lp/lp_internals
        lp/lp_solver
        lp/solver_interface
    DEPENDS
        named_vector
    DEPENDENCY_ONLY
)
if(USE_LP)
    find_package(Cplex 12)
    if(CPLEX_FOUND)
        target_compile_definitions(lp_solver INTERFACE HAS_CPLEX)
        target_link_libraries(lp_solver INTERFACE cplex::cplex)
        target_sources(lp_solver INTERFACE lp/cplex_solver_interface.h lp/cplex_solver_interface.cc)
    endif()

    find_package(soplex 7.1.0 QUIET)
    if (SOPLEX_FOUND)
        message(STATUS "Found SoPlex: ${SOPLEX_INCLUDE_DIRS}")
        target_link_libraries(lp_solver INTERFACE libsoplex)
        target_compile_definitions(lp_solver INTERFACE HAS_SOPLEX)
        target_sources(lp_solver INTERFACE lp/soplex_solver_interface.h lp/soplex_solver_interface.cc)
    endif()
endif()

create_fast_downward_library(
    NAME relaxation_heuristic
    HELP "The base class for relaxation heuristics"
    SOURCES
        heuristics/array_pool
        heuristics/relaxation_heuristic
    DEPENDS
        default_value_axioms_task
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME additive_heuristic
    HELP "The additive heuristic"
    SOURCES
        heuristics/additive_heuristic
    DEPENDS
        priority_queues
        relaxation_heuristic
        task_properties
)

create_fast_downward_library(
    NAME blind_search_heuristic
    HELP "The 'blind search' heuristic"
    SOURCES
        heuristics/blind_search_heuristic
    DEPENDS
        task_properties
)

create_fast_downward_library(
    NAME context_enhanced_additive_heuristic
    HELP "The context-enhanced additive heuristic"
    SOURCES
        heuristics/cea_heuristic
    DEPENDS
        default_value_axioms_task
        domain_transition_graph
        priority_queues
        task_properties
)

create_fast_downward_library(
    NAME cg_heuristic
    HELP "The causal graph heuristic"
    SOURCES heuristics/cg_heuristic
            heuristics/cg_cache
    DEPENDS
        default_value_axioms_task
        domain_transition_graph
        priority_queues
        task_properties
)

create_fast_downward_library(
    NAME domain_transition_graph
    HELP "DTGs used by cg and cea heuristic"
    SOURCES
        heuristics/domain_transition_graph
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME ff_heuristic
    HELP "The FF heuristic (an implementation of the RPG heuristic)"
    SOURCES
        heuristics/ff_heuristic
    DEPENDS
        additive_heuristic
        task_properties
)

create_fast_downward_library(
    NAME goal_count_heuristic
    HELP "The goal-counting heuristic"
    SOURCES
        heuristics/goal_count_heuristic
)

create_fast_downward_library(
    NAME hm_heuristic
    HELP "The h^m heuristic"
    SOURCES
        heuristics/hm_heuristic
    DEPENDS
        task_properties
)

create_fast_downward_library(
    NAME landmark_cut_heuristic
    HELP "The LM-cut heuristic"
    SOURCES
        heuristics/lm_cut_heuristic
        heuristics/lm_cut_landmarks
    DEPENDS
        priority_queues
        task_properties
)

create_fast_downward_library(
    NAME max_heuristic
    HELP "The Max heuristic"
    SOURCES
        heuristics/max_heuristic
    DEPENDS
        priority_queues
        relaxation_heuristic
)

create_fast_downward_library(
    NAME core_tasks
    HELP "Core task transformations"
    SOURCES
        tasks/cost_adapted_task
        tasks/delegating_task
        tasks/root_task
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME extra_tasks
    HELP "Non-core task transformations"
    SOURCES
        tasks/domain_abstracted_task
        tasks/domain_abstracted_task_factory
        tasks/modified_goals_task
        tasks/modified_operator_costs_task
    DEPENDS
        task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME default_value_axioms_task
    HELP "Task transformation adding axioms describing under which circumstances a derived variable is set to its default value."
    SOURCES
        tasks/default_value_axioms_task
    DEPENDS
        core_tasks
        sccs
        task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME causal_graph
    HELP "Causal Graph"
    SOURCES
        task_utils/causal_graph
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME sampling
    HELP "Sampling"
    SOURCES
        task_utils/sampling
    DEPENDS
        successor_generator
        task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME successor_generator
    HELP "Successor generator"
    SOURCES
        task_utils/successor_generator
        task_utils/successor_generator_factory
        task_utils/successor_generator_internals
    DEPENDS
        task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME task_properties
    HELP "Task properties"
    SOURCES
        task_utils/task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME variable_order_finder
    HELP "Variable order finder"
    SOURCES
        task_utils/variable_order_finder
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME cegar
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
    DEPENDS
        additive_heuristic
        dynamic_bitset
        extra_tasks
        landmarks
        priority_queues
        task_properties
)

create_fast_downward_library(
    NAME mas_heuristic
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
    DEPENDS
        priority_queues
        equivalence_relation
        sccs
        task_properties
        variable_order_finder
)

create_fast_downward_library(
    NAME landmarks
    HELP "Plugin containing the code to reason with landmarks"
    SOURCES
        landmarks/exploration
        landmarks/landmark
        landmarks/landmark_cost_partitioning_algorithms
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
    DEPENDS
        default_value_axioms_task
        lp_solver
        priority_queues
        successor_generator
        task_properties
)

create_fast_downward_library(
    NAME operator_counting
    HELP "Plugin containing the code for operator-counting heuristics"
    SOURCES
        operator_counting/constraint_generator
        operator_counting/delete_relaxation_if_constraints
        operator_counting/delete_relaxation_rr_constraints
        operator_counting/lm_cut_constraints
        operator_counting/operator_counting_heuristic
        operator_counting/pho_constraints
        operator_counting/state_equation_constraints
    DEPENDS lp_solver landmark_cut_heuristic pdbs task_properties
)

create_fast_downward_library(
    NAME pdbs
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
    DEPENDS
        causal_graph
        max_cliques
        priority_queues
        sampling
        successor_generator
        task_properties
        variable_order_finder
)

create_fast_downward_library(
    NAME potentials
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
    DEPENDS
        lp_solver
        sampling
        successor_generator
        task_properties
)

create_fast_downward_library(
    NAME sccs
    HELP "Algorithm to compute the strongly connected components (SCCs) of a directed graph."
    SOURCES
        algorithms/sccs
    DEPENDENCY_ONLY
)
