cmake_minimum_required(VERSION 2.4)

project(search)

### Fast Downward CMake build file
### Unless someone adopts it, queries/comments to Andrew.Coles@cis.strath.ac.uk

## == Path containing some helper CMake modules ==

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

## == Definition of build flags ==
##
## You probably won't need to change this system, unless you want to tweak it for a specific
## platform.  By default, four build targets are defined:
##
## * -DCMAKE_BUILD_TYPE=Debug         - no optimisation, full debugging information
## * -DCMAKE_BUILD_TYPE=Release       - -O3 optimisation, static binary, no debugging information
## * -DCMAKE_BUILD_TYPE=RelWithDeInfo - -O3 optimisation, static binary, but with debugging information
## * -DCMAKE_BUILD_TYPE=Profile       - -O3 optimisation, profile information linked in
##

# Target-agnostic build flags, taken from the legacy downward Makefile.

if(NOT STATE_VAR_BYTES)
    message("** Defaulting to STATE_VAR_BYTES=1 **")
    set(STATE_VAR_BYTES 1)
endif()

set(TARGET "downward-${STATE_VAR_BYTES}")

set(CMAKE_CXX_FLAGS "-Iext -m32 -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -DSTATE_VAR_BYTES=${STATE_VAR_BYTES}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -m32")

# Default to Release build, unless specified otherwise

if(NOT CMAKE_BUILD_TYPE)
    #message("** Defaulting to Release build **")
    set(CMAKE_BUILD_TYPE "RELEASE")
endif()


# Here, we modify the default Release build compile flags to make a static executable
# This is done slightly differently for different compilers: the -static-libgcc flag
# makes no sense if using the Intel compiler.

# WARNING: Note that unlike the legacy Makefile, -g is *not enabled* in Release build.
#          Either use the RelWithDebInfo build, or add -g to the line below.


# First, make a lower-case version of the build type

string(TOLOWER "${CMAKE_BUILD_TYPE}" LOWER_CASE_BUILD_TYPE)

# Note that we should use static-linkage for the two sorts of release build

if(LOWER_CASE_BUILD_TYPE STREQUAL "release" OR LOWER_CASE_BUILD_TYPE STREQUAL "relwithdebinfo")
   set(STATIC_BUILD TRUE)
endif(LOWER_CASE_BUILD_TYPE STREQUAL "release" OR LOWER_CASE_BUILD_TYPE STREQUAL "relwithdebinfo")


if(CMAKE_COMPILER_IS_GNUCXX)

    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3")

    if(STATIC_BUILD)
        #message("-- Building statically linked executable")
        set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static -static-libgcc")
    endif(STATIC_BUILD)

else(CMAKE_COMPILER_IS_GNUCXX)

    # Sensible defaults for other compilers

    message(WARNING "Not using GNU compiler - currently untested")

    set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG -fomit-frame-pointer")
    set(CMAKE_CXX_LINK_FLAGS_RELEASE "-static")

    if(STATIC_BUILD)
        #message("-- Building statically linked executable")
        set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static")
    endif(STATIC_BUILD)

endif(CMAKE_COMPILER_IS_GNUCXX)

# Set the desired level of debugging for the Debug build, and to link in the rt library

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_LINK_FLAGS_DEBUG "-g")

# Define the profile build.  Incidentally, cMake doesn't have one of these by default,
# so this also demonstrates that it's fairly easy to add a custom build type (e.g. if you
# wanted a specific optimisation--debugging--static combination).

set(CMAKE_CXX_FLAGS_PROFILE "-pg -O3")
set(CMAKE_CXX_LINK_FLAGS_PROFILE "-pg")

# Finally, find the rt library for the exact timer

find_library(RT_LIBRARY rt)

## == Finding (and if necessary, compiling) the LP libraries ==

## Note that adding the libraries as dependencies of the search executable target is done
## lower down - this just handles finding the necessary files.

option(USE_LP "Whether to enable code that requires an LP solver" FALSE)
option(USE_LP_INTERNAL "Whether to compile, at cmake time, the Osi installation shipped with the sources, and use this rather than the system libraries.  If setting this to false, make sure you have static versions of the libraries available if you intend to use the resulting binaries in a scenario where that matters." TRUE)

if(USE_LP)

    if(USE_LP_INTERNAL)

        # If we get this far, then the LP stuff has been enabled AND we need to compile the shipped version of Osi
        # This is handled by a makefile sitting in the lp sub-directory, which calls the setup script if
        # lp/coin doesn't exist (the path into which the compiled Osi release is installed)

        exec_program(make "${CMAKE_CURRENT_SOURCE_DIR}/lp")

        # The cmake modules for finding the Osi components look in LP_PREFIX_PATH before the system
        # defaults.  Putting this here means the same modules can be used, irrespective of whether
        # the system or supplied version of Osi is used.

        set(LP_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lp/coin")

        mark_as_advanced(LP_PREFIX_PATH)

        # Each of these is defined as a file called FindFoo.cmake

        find_package(OSI REQUIRED)
        find_package(Clp REQUIRED)
        find_package(CoinUtils REQUIRED)

    else(USE_LP_INTERNAL)

        find_package(OSI REQUIRED)
        find_package(Clp REQUIRED)
        find_package(CoinUtils REQUIRED)

    endif(USE_LP_INTERNAL)



    # We define here that the appropriate include paths are used.  Note, however, that we do not need
    # to define library paths as e.g. ${CLP_LIBRARIES} contains an absolute path to a CLP library.

    include_directories(${OSI_INCLUDES}/coin ${CLP_INCLUDES}/coin ${COINUTILS_INCLUDES}/coin)

endif(USE_LP)


## == Details of the core source files ==
##
## If you're adding another .cc file to the codebase which *isn't* a plugin, add
## it to the following list

set(search_main_SRCS
          axioms.cc
          causal_graph.cc
          closed_list.cc
          combining_evaluator.cc
          domain_transition_graph.cc
          eager_search.cc
          enforced_hill_climbing_search.cc
          exact_timer.cc
          g_evaluator.cc
          globals.cc
          heuristic.cc
          ipc_max_heuristic.cc
          iterated_search.cc
          lazy_search.cc
          max_evaluator.cc
          operator.cc
          operator_cost.cc
          option_parser.cc
          pref_evaluator.cc
          relaxation_heuristic.cc
          rng.cc
          search_engine.cc
          search_node_info.cc
          search_progress.cc
          search_space.cc
          state.cc
          successor_generator.cc
          sum_evaluator.cc
          timer.cc
          utilities.cc
          weighted_evaluator.cc
          open_lists/alternation_open_list.cc
          open_lists/open_list_buckets.cc
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
endif(PLUGIN_ADDITIVE_HEURISTIC)

option(PLUGIN_BLIND_SEARCH_HEURISTIC "The 'blind search' heuristic" TRUE)
if(PLUGIN_BLIND_SEARCH_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} blind_search_heuristic.cc)
endif(PLUGIN_BLIND_SEARCH_HEURISTIC)


option(PLUGIN_CEA_HEURISTIC "The context-enhanced additive heuristic" TRUE)
if(PLUGIN_CEA_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} cea_heuristic.cc)
endif(PLUGIN_CEA_HEURISTIC)


option(PLUGIN_CG_HEURISTIC "The causal graph heuristic" TRUE)
if(PLUGIN_CG_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} cg_heuristic.cc cg_cache.cc)
endif(PLUGIN_CG_HEURISTIC)


option(PLUGIN_FF_HEURISTIC "The FF heuristic (an implementation of the RPG heuristic)" TRUE)
if(PLUGIN_FF_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} ff_heuristic.cc)
endif(PLUGIN_FF_HEURISTIC)


option(PLUGIN_GOAL_COUNT_HEURISTIC "The goal-counting heuristic" TRUE)
if(PLUGIN_GOAL_COUNT_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} goal_count_heuristic.cc)
endif(PLUGIN_GOAL_COUNT_HEURISTIC)


option(PLUGIN_HM_HEURISTIC "The h^m heuristic" TRUE)
if(PLUGIN_HM_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} hm_heuristic.cc)
endif(PLUGIN_HM_HEURISTIC)


option(PLUGIN_LM_CUT_HEURISTIC "The LM-cut heuristic" TRUE)
if(PLUGIN_LM_CUT_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} lm_cut_heuristic.cc)
endif(PLUGIN_LM_CUT_HEURISTIC)


option(PLUGIN_MAX_HEURISTIC "The Max heuristic" TRUE)
if(PLUGIN_MAX_HEURISTIC)
    set(search_plugin_SRCS ${search_plugin_SRCS} max_heuristic.cc)
endif(PLUGIN_MAX_HEURISTIC)


option(PLUGIN_MAS_HEURISTIC "The Merge-and-Shrink heuristic" TRUE)
if(PLUGIN_MAS_HEURISTIC)

    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        merge_and_shrink/abstraction.cc
        merge_and_shrink/label_reducer.cc
        merge_and_shrink/merge_and_shrink_heuristic.cc
        merge_and_shrink/shrink_bisimulation.cc
        merge_and_shrink/shrink_bucket_based.cc
        merge_and_shrink/shrink_fh.cc
        merge_and_shrink/shrink_random.cc
        merge_and_shrink/shrink_strategy.cc
        merge_and_shrink/variable_order_finder.cc
    )

endif(PLUGIN_MAS_HEURISTIC)


option(PLUGIN_LANDMARKS "Plugin containing the code to reason with landmarks" TRUE)
if(PLUGIN_LANDMARKS)

    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        landmarks/exploration.cc
        landmarks/h_m_landmarks.cc
        landmarks/lama_ff_synergy.cc
        landmarks/landmark_cost_assignment.cc
        landmarks/landmark_count_heuristic.cc
        landmarks/landmark_status_manager.cc
        landmarks/landmark_graph_merged.cc
        landmarks/landmark_graph.cc
        landmarks/landmark_factory.cc
        landmarks/landmark_factory_rpg_exhaust.cc
        landmarks/landmark_factory_rpg_sasp.cc
        landmarks/landmark_factory_zhu_givan.cc
        landmarks/util.cc
    )

endif(PLUGIN_LANDMARKS)


option(PLUGIN_LEARNING "Plugin containing the code to reason with learning" TRUE)
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

endif(PLUGIN_LEARNING)


option(PLUGIN_PDBS "Plugin containing the code for PDBs" TRUE)
if(PLUGIN_PDBS)

    set(search_plugin_SRCS
        ${search_plugin_SRCS}
        pdbs/canonical_pdbs_heuristic.cc
        pdbs/match_tree.cc
        pdbs/max_cliques.cc
        pdbs/pattern_generation_edelkamp.cc
        pdbs/pattern_generation_haslum.cc
        pdbs/pdb_heuristic.cc
        pdbs/util.cc
        pdbs/zero_one_pdbs_heuristic.cc
    )

endif(PLUGIN_PDBS)


## == Defining the executable target ==

## After having defined the variables containing the names of the source files, this bit is remarkably non-eventful

add_executable(${TARGET}
    planner.cc
    ${search_main_SRCS}
    ${search_plugin_SRCS}
)


## ... until we consider the additional definitions and libraries needed in the LP case

if(NOT USE_LP)
    target_link_libraries(${TARGET}
        ${RT_LIBRARY}
    )

else(NOT USE_LP)

    set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-D USE_LP -D COIN_USE_CLP")

    target_link_libraries(${TARGET}
        ${RT_LIBRARY}
        ${OSICLP_LIBRARIES} ${CLP_LIBRARIES}  ${COINUTILS_LIBRARIES} ${OSI_LIBRARIES}
    )

    ## WARNING: In the original Makefile, there were lines manipulating the linker flags by adding -Wl,-Bstatic and suchlike.
    ##          These have gone, replaced by passing --enable-shared=no to the Osi build to make sure only static libraries
    ##          exist in the first place.


endif(NOT USE_LP)
