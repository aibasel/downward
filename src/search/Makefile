## Variables prefixed with "DOWNWARD_" are used to configure the 
## build. They can be changed on the command line or set as 
## environment variables.

## By default, build in 32-bit mode. Use DOWNWARD_BITWIDTH=64
## to build in 64-bit mode and DOWNWARD_BITWIDTH=native to use
## the native bitness of the OS.
DOWNWARD_BITWIDTH ?= 32

## Set DOWNWARD_USE_LP to 1 to enable linear programming stuff.
DOWNWARD_USE_LP ?= 0

## Fast Downward uses the Open Solver Interface (OSI) by the
## COIN-OR project for access to LP solvers. Set DOWNWARD_COIN_ROOT
## to a path where OSI is installed.
DOWNWARD_COIN_ROOT ?= /opt/coin

## If you want to use the CPLEX and/or Gurobi LP solvers, set the
## following variables to paths for the "include" and "lib"
## directories of the solvers. 
DOWNWARD_CPLEX_INCDIR ?= /opt/ibm/ILOG/CPLEX_Studio1251/cplex/include/ilcplex
DOWNWARD_CPLEX_LIBDIR ?= /opt/ibm/ILOG/CPLEX_Studio1251/cplex/lib/x86_sles10_4.1/static_pic
DOWNWARD_GUROBI_INCDIR ?=
DOWNWARD_GUROBI_LIBDIR ?=


## Set DOWNWARD_LINK_RELEASE_STATICALLY to 0 or 1 (default) to
## disable/enable static linking of the executable in release mode.
## On OS X, this is unsupported and will be silently disabled.
DOWNWARD_LINK_RELEASE_STATICALLY ?= 1

## On a supported operating system, there should be no need to override
## the DOWNWARD_OS setting. If the provided code does not work even
## though your operating system is a supported one, please report this
## as a bug.
DOWNWARD_OS ?= auto


HEADERS = \
          axioms.h \
          causal_graph.h \
          combining_evaluator.h \
          domain_transition_graph.h \
          eager_search.h \
          enforced_hill_climbing_search.h \
          equivalence_relation.h \
          exact_timer.h \
          g_evaluator.h \
          globals.h \
          heuristic.h \
          int_packer.h \
          ipc_max_heuristic.h \
          iterated_search.h \
          lazy_search.h \
          legacy_causal_graph.h \
          max_evaluator.h \
          operator.h \
          operator_cost.h \
          option_parser.h \
          option_parser_util.h \
          segmented_vector.h \
          per_state_information.h \
          pref_evaluator.h \
          relaxation_heuristic.h \
          rng.h \
          search_engine.h \
          search_node_info.h \
          search_progress.h \
          search_space.h \
          state.h \
          state_id.h \
          state_registry.h \
          successor_generator.h \
          sum_evaluator.h \
          timer.h \
          utilities.h \
          weighted_evaluator.h \
          \
          open_lists/alternation_open_list.h \
          open_lists/open_list_buckets.h \
          open_lists/pareto_open_list.h \
          open_lists/standard_scalar_open_list.h \
          open_lists/tiebreaking_open_list.h \
          \
          lp_solver_interface.h \

## Each of the following "HEADERS += ..." constructs defines a
## "plugin" feature that can be enabled or disabled by simply
## commenting out the respective lines in the Makefile.

## As with all changes to the Makefile, you will have to force a
## rebuild after such a change. Deleting the executable and running
## "make" to relink is enough; no need to do a complete rebuild.

HEADERS += additive_heuristic.h
HEADERS += blind_search_heuristic.h
HEADERS += cea_heuristic.h
HEADERS += cg_heuristic.h cg_cache.h
HEADERS += ff_heuristic.h
HEADERS += goal_count_heuristic.h
HEADERS += hm_heuristic.h
HEADERS += lm_cut_heuristic.h
HEADERS += max_heuristic.h

HEADERS += merge_and_shrink/abstraction.h \
           merge_and_shrink/label.h \
           merge_and_shrink/labels.h \
           merge_and_shrink/label_reducer.h \
           merge_and_shrink/merge_and_shrink_heuristic.h \
           merge_and_shrink/merge_dfp.h \
           merge_and_shrink/merge_linear.h \
           merge_and_shrink/merge_strategy.h \
           merge_and_shrink/shrink_bisimulation.h \
           merge_and_shrink/shrink_bucket_based.h \
           merge_and_shrink/shrink_fh.h \
           merge_and_shrink/shrink_random.h \
           merge_and_shrink/shrink_strategy.h \
           merge_and_shrink/variable_order_finder.h \

HEADERS += landmarks/exploration.h \
           landmarks/h_m_landmarks.h \
           landmarks/lama_ff_synergy.h \
           landmarks/landmark_cost_assignment.h \
           landmarks/landmark_count_heuristic.h \
           landmarks/landmark_status_manager.h \
           landmarks/landmark_graph_merged.h \
           landmarks/landmark_graph.h \
           landmarks/landmark_factory.h \
           landmarks/landmark_factory_rpg_exhaust.h \
           landmarks/landmark_factory_rpg_sasp.h \
           landmarks/landmark_factory_zhu_givan.h \
           landmarks/util.h \

# HEADERS += learning/AODE.h \
#            learning/classifier.h \
#            learning/composite_feature_extractor.h \
#            learning/feature_extractor.h \
#            learning/maximum_heuristic.h \
#            learning/naive_bayes_classifier.h \
#            learning/PDB_state_space_sample.h \
#            learning/probe_state_space_sample.h \
#            learning/selective_max_heuristic.h \
#            learning/state_space_sample.h \
#            learning/state_vars_feature_extractor.h \

HEADERS += pdbs/canonical_pdbs_heuristic.h \
           pdbs/dominance_pruner.h \
           pdbs/match_tree.h \
           pdbs/max_cliques.h \
           pdbs/pattern_generation_edelkamp.h \
           pdbs/pattern_generation_haslum.h \
           pdbs/pdb_heuristic.h \
           pdbs/util.h \
           pdbs/zero_one_pdbs_heuristic.h \

SOURCES = planner.cc $(HEADERS:%.h=%.cc)
TARGET = downward

default: release

ARGS_PROFILE = --search 'astar(lmcut())' < profile-input.pre

SHELL = /bin/bash

ifeq ($(DOWNWARD_OS), auto)
    UNAME := $(shell uname)

    ifeq ($(UNAME), Darwin)
        DOWNWARD_OS=osx
    else ifeq ($(UNAME), Linux)
        DOWNWARD_OS=linux
    else
        UNAME_O := $(shell uname -o)
    endif

    ifeq ($(UNAME_O), Cygwin)
        ## For now, we treat Linux and Cygwin equally in this
        ## Makefile, so we just call Cygwin "linux".
        DOWNWARD_OS=linux
    else ifeq ($(OS), auto)
        $(warning OS detection failed -- setting to Linux and hoping for the best!)
        DOWNWARD_OS=linux
    endif
endif

ifeq ($(DOWNWARD_OS), osx)
    ## Disable static linking on OS X.
    DOWNWARD_LINK_RELEASE_STATICALLY=0
endif


OBJECT_SUFFIX_RELEASE = .release
TARGET_SUFFIX_RELEASE = -release
OBJECT_SUFFIX_DEBUG   = .debug
TARGET_SUFFIX_DEBUG   = -debug
OBJECT_SUFFIX_PROFILE = .profile
TARGET_SUFFIX_PROFILE = -profile

OBJECTS_RELEASE = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_RELEASE).o)
TARGET_RELEASE  = $(TARGET)$(TARGET_SUFFIX_RELEASE)

OBJECTS_DEBUG   = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_DEBUG).o)
TARGET_DEBUG    = $(TARGET)$(TARGET_SUFFIX_DEBUG)

OBJECTS_PROFILE = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_PROFILE).o)
TARGET_PROFILE  = $(TARGET)$(TARGET_SUFFIX_PROFILE)

DEPEND = $(CXX) -MM

## CXXFLAGS, LDFLAGS, POSTLINKOPT are options for compiler and linker
## that are used for all three targets (release, debug, and profile).
## (POSTLINKOPT are options that appear *after* all object files.)

ifeq ($(DOWNWARD_BITWIDTH), 32)
    BITWIDTHOPT = -m32
else ifeq ($(DOWNWARD_BITWIDTH), 64)
    BITWIDTHOPT = -m64
else ifneq ($(DOWNWARD_BITWIDTH), native)
    $(error Bad value for BITWIDTH)
endif

CXXFLAGS =
CXXFLAGS += -g
CXXFLAGS += $(BITWIDTHOPT)
CXXFLAGS += -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror

CXXFLAGS += -Iext
ifeq ($(DOWNWARD_OS), osx)
    ## If you have Mac OS, you may be getting tree.hh from /opt/local/include.
    CXXFLAGS += -I/opt/local/include/
endif


## The following lines contain workarounds for bugs when
## cross-compiling to 64 bit on 32-bit systems using gcc 4.4 or gcc
## 4.5 in some Ubuntu releases. (We don't usually cross-compile to
## 64-bit, but in some cases we do; e.g. we did for the IPC.) See
## http://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit.
ifeq ($(DOWNWARD_OS), linux) # workarounds only valid for Linux...
    HAVE_GCC_4_4 := $(shell expr "$$(gcc -dumpversion)" : \\\(4\.4\.\\\))
    HAVE_GCC_4_5 := $(shell expr "$$(gcc -dumpversion)" : \\\(4\.5\.\\\))

    ifdef HAVE_GCC_4_4
        CXXFLAGS += -I/usr/include/c++/4.4/i686-linux-gnu
    endif

    ifdef HAVE_GCC_4_5
        CXXFLAGS += -I/usr/include/c++/4.5/i686-linux-gnu
    endif
endif

LDFLAGS =
LDFLAGS += $(BITWIDTHOPT)
LDFLAGS += -g

POSTLINKOPT =

## Additional specialized options for the various targets follow.
## In release mode, we link statically since this makes it more likely
## that local compiles will work on the various grids (gkigrid, Black
## Forest Grid, maia).
##
## NOTE: This precludes some uses of exceptions.
##        For details, see man gcc on -static-libgcc.

CXXFLAGS_RELEASE  = -O3 -DNDEBUG -fomit-frame-pointer
CXXFLAGS_DEBUG    = -O3
CXXFLAGS_PROFILE  = -O3 -pg

LDFLAGS_RELEASE  =
LDFLAGS_DEBUG    =
LDFLAGS_PROFILE  = -pg

POSTLINKOPT_RELEASE =
POSTLINKOPT_DEBUG   =
POSTLINKOPT_PROFILE =

ifeq ($(DOWNWARD_LINK_RELEASE_STATICALLY), 1)
    LDFLAGS_RELEASE += -static -static-libgcc
endif

ifeq ($(DOWNWARD_OS), linux)
    ifeq ($(DOWNWARD_LINK_RELEASE_STATICALLY), 0)
        POSTLINKOPT_RELEASE += -lrt
    else
        POSTLINKOPT_RELEASE += -Wl,-Bstatic -lrt
    endif
    POSTLINKOPT_DEBUG  += -lrt
    POSTLINKOPT_PROFILE += -lrt
endif


ifeq ($(DOWNWARD_USE_LP),1)

$(DOWNWARD_COIN_ROOT):
	$(error COIN is no longer bundled with Fast Downward. Please install COIN outside of the repository and set up the environment variable DOWNWARD_COIN_ROOT to the new path. See http://www.fast-downward.org/LPBuildInstructions for details on how to install COIN and LP solvers)

$(TARGET_RELEASE) $(OBJECTS_RELEASE): $(DOWNWARD_COIN_ROOT)
$(TARGET_DEBUG) $(OBJECTS_DEBUG): $(DOWNWARD_COIN_ROOT)
$(TARGET_PROFILE) $(OBJECTS_PROFILE): $(DOWNWARD_COIN_ROOT)

## We want to link the Linear Programming libraries statically since
## they are unlikely to be preinstalled on the grids we use for
## evaluation. Static linking is a bit tricky: we need to specify the
## libraries *after* the source files and in such an order that if A
## depends on B, A is listed before B. (In case of dependency cycles,
## we can and must list them multiple times.) The following set of
## libraries and their ordering have been determined experimentally
## and hence might break if we use more functions from the LP
## libraries. See
## http://ask.metafilter.com/117792/How-to-fix-C-static-linking-problems

COIN_LIBS =
COIN_CXXFLAGS =
COIN_LDFLAGS =

COIN_HAS_CPX := $(shell test -d $(DOWNWARD_CPLEX_INCDIR) && test -f $(DOWNWARD_CPLEX_LIBDIR)/libcplex.a && test -f $(DOWNWARD_COIN_ROOT)/lib/libOsiCpx.a && echo 1)
ifeq ($(COIN_HAS_CPX),1)
COIN_LIBS += OsiCpx cplex
COIN_CXXFLAGS += -D COIN_HAS_CPX
COIN_LDFLAGS += -L$(DOWNWARD_CPLEX_LIBDIR) -pthread
endif

COIN_HAS_GRB := $(shell test -d $(DOWNWARD_GUROBI_INCDIR) && test -f $(DOWNWARD_GUROBI_LIBDIR)/libgurobi.a && test -f $(DOWNWARD_COIN_ROOT)/lib/OsiGrb.a && echo 1)
ifeq ($(COIN_HAS_GRB),1)
COIN_LIBS += OsiGrb gurobi
COIN_CXXFLAGS += -D COIN_HAS_GRB
COIN_LDFLAGS += -L$(DOWNWARD_GUROBI_LIBDIR) -pthread
endif

## CLP is part of coin and can always be enabled
COIN_CXXFLAGS += -D COIN_HAS_CLP
COIN_LIBS += OsiClp Clp

## Basic Osi libs must be added after (!) all osi solver libs
COIN_LIBS += Osi CoinUtils

## We want to always link the COIN libraries statically, even if static
## linking is otherwise disabled. We accomplish this by
## using -Wl,-Bstatic before the COIN libs and -Wl,-Bdynamic
## afterwards (unless in release mode with static linking enabled). See
## http://ubuntuforums.org/showthread.php?t=491455

COIN_CXXFLAGS += -I$(DOWNWARD_COIN_ROOT)/include/coin -D USE_LP
ifeq ($(OS), osx)
COIN_LDFLAGS += -L$(DOWNWARD_COIN_ROOT)/lib $(COIN_LIBS:%=-l %)
else
COIN_LDFLAGS += -L$(DOWNWARD_COIN_ROOT)/lib -Wl,-Bstatic $(COIN_LIBS:%=-l %)
endif

CXXFLAGS += $(COIN_CXXFLAGS)

POSTLINKOPT_RELEASE += $(COIN_LDFLAGS)
POSTLINKOPT_DEBUG += $(COIN_LDFLAGS)
POSTLINKOPT_PROFILE += $(COIN_LDFLAGS)

ifeq ($(DOWNWARD_OS), linux)
ifeq ($(DOWNWARD_LINK_RELEASE_STATICALLY), 0)
POSTLINKOPT_RELEASE += -Wl,-Bdynamic
endif
POSTLINKOPT_DEBUG   += -Wl,-Bdynamic
POSTLINKOPT_PROFILE += -Wl,-Bdynamic
endif

endif # LP code

all: release debug profile

## Build rules for the release target follow.

release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJECTS_RELEASE)
	$(CXX) $(LDFLAGS) $(LDFLAGS_RELEASE) $(OBJECTS_RELEASE) $(POSTLINKOPT) $(POSTLINKOPT_RELEASE) -o $(TARGET_RELEASE)

$(OBJECTS_RELEASE): .obj/%$(OBJECT_SUFFIX_RELEASE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_RELEASE) -c $< -o $@

## Build rules for the debug target follow.

debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJECTS_DEBUG)
	$(CXX) $(LDFLAGS) $(LDFLAGS_DEBUG) $(OBJECTS_DEBUG) $(POSTLINKOPT) $(POSTLINKOPT_DEBUG) -o $(TARGET_DEBUG)

$(OBJECTS_DEBUG): .obj/%$(OBJECT_SUFFIX_DEBUG).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_DEBUG) -c $< -o $@

## Build rules for the profile target follow.

profile: $(TARGET_PROFILE)

$(TARGET_PROFILE): $(OBJECTS_PROFILE)
	$(CXX) $(LDFLAGS) $(LDFLAGS_PROFILE) $(OBJECTS_PROFILE) $(POSTLINKOPT) $(POSTLINKOPT_PROFILE) -o $(TARGET_PROFILE)

$(OBJECTS_PROFILE): .obj/%$(OBJECT_SUFFIX_PROFILE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_PROFILE) -c $< -o $@

## Additional targets follow.

PROFILE: $(TARGET_PROFILE)
	./$(TARGET_PROFILE) $(ARGS_PROFILE)
	gprof $(TARGET_PROFILE) | (cleanup-profile 2> /dev/null || cat) > PROFILE

clean:
	rm -rf .obj
	rm -f *~ *.pyc
	rm -f Makefile.depend gmon.out PROFILE core
	rm -f sas_plan

distclean: clean
	rm -f $(TARGET_RELEASE) $(TARGET_DEBUG) $(TARGET_PROFILE)

## NOTE: If we just call gcc -MM on a source file that lives within a
## subdirectory, it will strip the directory part in the output. Hence
## the for loop with the sed call.

Makefile.depend: $(SOURCES) $(HEADERS)
	rm -f Makefile.temp
	for source in $(SOURCES) ; do \
	    $(DEPEND) $(CXXFLAGS) $$source > Makefile.temp0; \
	    objfile=$${source%%.cc}.o; \
	    sed -i -e "s@^[^:]*:@$$objfile:@" Makefile.temp0; \
	    cat Makefile.temp0 >> Makefile.temp; \
	done
	rm -f Makefile.temp0 Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_RELEASE).o:\2@" Makefile.temp >> Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_DEBUG).o:\2@" Makefile.temp >> Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_PROFILE).o:\2@" Makefile.temp >> Makefile.depend
	rm -f Makefile.temp

ifneq ($(MAKECMDGOALS),clean)
    ifneq ($(MAKECMDGOALS),distclean)
        -include Makefile.depend
    endif
endif

.PHONY: default all release debug profile clean distclean
