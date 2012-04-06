## Set USE_LP to 1 to enable linear programming stuff.
USE_LP=0

## Set LINK_RELEASE_STATICALLY to 0 or 1 (default) to disable/enable
## static linking of the executable in release mode.
## On OS X, this is unsupported and will be silently disabled.
LINK_RELEASE_STATICALLY=1

## Set STATE_VAR_BYTES to 1, 2 or 4 to control how large a state
## variable is. There should be no need to tweak this manually.
STATE_VAR_BYTES=1

## On a supported operating system, there should be no need to override
## the OS setting. If the provided code does not work even though your
## operating system is a supported one, please report this as a bug.
OS=auto


HEADERS = \
          axioms.h \
          causal_graph.h \
          closed_list.h \
          combining_evaluator.h \
          domain_transition_graph.h \
          eager_search.h \
          enforced_hill_climbing_search.h \
          exact_timer.h \
          g_evaluator.h \
          globals.h \
          heuristic.h \
          ipc_max_heuristic.h \
          iterated_search.h \
          lazy_search.h \
          max_evaluator.h \
          operator.h \
          operator_cost.h \
          option_parser.h \
          pref_evaluator.h \
          relaxation_heuristic.h \
          rng.h \
          search_engine.h \
          search_node_info.h \
          search_progress.h \
          search_space.h \
          state.h \
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
           merge_and_shrink/label_reducer.h \
           merge_and_shrink/merge_and_shrink_heuristic.h \
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

HEADERS += learning/AODE.h \
           learning/classifier.h \
           learning/composite_feature_extractor.h \
           learning/feature_extractor.h \
           learning/maximum_heuristic.h \
           learning/naive_bayes_classifier.h \
           learning/PDB_state_space_sample.h \
           learning/probe_state_space_sample.h \
           learning/selective_max_heuristic.h \
           learning/state_space_sample.h \
           learning/state_vars_feature_extractor.h \

HEADERS += pdbs/canonical_pdbs_heuristic.h \
           pdbs/match_tree.h \
           pdbs/max_cliques.h \
           pdbs/pattern_generation_edelkamp.h \
           pdbs/pattern_generation_haslum.h \
           pdbs/pdb_heuristic.h \
           pdbs/util.h \
           pdbs/zero_one_pdbs_heuristic.h \

SHELL = /bin/bash

SOURCES = planner.cc $(HEADERS:%.h=%.cc)
TARGET  = downward

ARGS_PROFILE = --search 'astar(lmcut())' < profile-input.pre


ifeq ($(OS), auto)
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
OS=osx
endif
ifeq ($(UNAME), Linux)
OS=linux
endif
ifeq ($(OS), auto)
$(warning OS detection failed -- setting to Linux and hoping for the best!)
OS=linux
endif
endif

ifeq ($(OS), osx)
## Disable static linking on OS X.
LINK_RELEASE_STATICALLY=0
endif


OBJECT_SUFFIX_RELEASE = .$(STATE_VAR_BYTES)
TARGET_SUFFIX_RELEASE = -$(STATE_VAR_BYTES)
OBJECT_SUFFIX_DEBUG   = .$(STATE_VAR_BYTES).debug
TARGET_SUFFIX_DEBUG   = -$(STATE_VAR_BYTES)-debug
OBJECT_SUFFIX_PROFILE = .$(STATE_VAR_BYTES).profile
TARGET_SUFFIX_PROFILE = -$(STATE_VAR_BYTES)-profile

OBJECTS_RELEASE = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_RELEASE).o)
TARGET_RELEASE  = $(TARGET)$(TARGET_SUFFIX_RELEASE)

OBJECTS_DEBUG   = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_DEBUG).o)
TARGET_DEBUG    = $(TARGET)$(TARGET_SUFFIX_DEBUG)

OBJECTS_PROFILE = $(SOURCES:%.cc=.obj/%$(OBJECT_SUFFIX_PROFILE).o)
TARGET_PROFILE  = $(TARGET)$(TARGET_SUFFIX_PROFILE)

CC     = g++
DEPEND = g++ -MM

## CCOPT, LINKOPT, POSTLINKOPT are options for compiler and linker
## that are used for all three targets (release, debug, and profile).
## (POSTLINKOPT are options that appear *after* all object files.)

CCOPT = -Iext
CCOPT += -g
CCOPT += -m32
CCOPT += -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -DSTATE_VAR_BYTES=$(STATE_VAR_BYTES)

## The following lines contain workarounds for bugs when
## cross-compiling to 64 bit on 32-bit systems using gcc 4.4 or gcc
## 4.5 in some Ubuntu releases. (We don't usually cross-compile to
## 64-bit, but in some cases we do; e.g. we did for the IPC.) See
## http://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit.

HAVE_GCC_4_4 := $(shell expr "$$(gcc -dumpversion)" : \\\(4\.4\.\\\))
HAVE_GCC_4_5 := $(shell expr "$$(gcc -dumpversion)" : \\\(4\.5\.\\\))

ifdef HAVE_GCC_4_4
    CCOPT += -I/usr/include/c++/4.4/i686-linux-gnu
endif

ifdef HAVE_GCC_4_5
    CCOPT += -I/usr/include/c++/4.5/i686-linux-gnu
endif

LINKOPT  = -g
LINKOPT += -m32
LINKOPT +=

POSTLINKOPT =

## Additional specialized options for the various targets follow.
## In release mode, we link statically since this makes it more likely
## that local compiles will work on the various grids (gkigrid, Black
## Forest Grid, maia).
##
## NOTE: This precludes some uses of exceptions.
##        For details, see man gcc on -static-libgcc.

CCOPT_RELEASE  = -O3 -DNDEBUG -fomit-frame-pointer
CCOPT_DEBUG    = -O3
CCOPT_PROFILE  = -O3 -pg

LINKOPT_RELEASE =
LINKOPT_DEBUG    =
LINKOPT_PROFILE  = -pg

POSTLINKOPT_RELEASE =
POSTLINKOPT_DEBUG   =
POSTLINKOPT_PROFILE =

ifeq ($(LINK_RELEASE_STATICALLY), 1)
LINKOPT_RELEASE += -static -static-libgcc
endif

ifeq ($(OS), linux)
ifeq ($(LINK_RELEASE_STATICALLY), 0)
LINKOPT_RELEASE += -lrt
else
POSTLINKOPT_RELEASE += -Wl,-Bstatic -lrt
endif
LINKOPT_DEBUG  += -lrt
LINKOPT_PROFILE += -lrt
# LINKOPT_PROFILE += -lc_p
endif


## Define the default target up here so that the LP stuff below
## doesn't define a default target.

default: release

ifeq ($(USE_LP),1)

COIN_ROOT = lp/coin

$(COIN_ROOT):
	cd lp && ./setup

$(TARGET_RELEASE) $(OBJECTS_RELEASE): $(COIN_ROOT)
$(TARGET_DEBUG) $(OBJECTS_DEBUG): $(COIN_ROOT)
$(TARGET_PROFILE) $(OBJECTS_PROFILE): $(COIN_ROOT)

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

COIN_LIBS = OsiClp Clp CoinUtils Osi

## We want to always link the COIN libraries statically, even if static
## linking is otherwise disabled. We accomplish this by
## using -Wl,-Bstatic before the COIN libs and -Wl,-Bdynamic
## afterwards (unless in release mode with static linking enabled). See
## http://ubuntuforums.org/showthread.php?t=491455

COIN_CCOPT = -I$(COIN_ROOT)/include/coin -D USE_LP -D COIN_USE_CLP
ifeq ($(OS), osx)
COIN_LINKOPT = -L$(COIN_ROOT)/lib $(COIN_LIBS:%=-l %)
else
COIN_LINKOPT = -L$(COIN_ROOT)/lib -Wl,-Bstatic $(COIN_LIBS:%=-l %)
endif

CCOPT += $(COIN_CCOPT)

POSTLINKOPT_RELEASE += $(COIN_LINKOPT)
POSTLINKOPT_DEBUG += $(COIN_LINKOPT)
POSTLINKOPT_PROFILE += $(COIN_LINKOPT)

ifeq ($(OS), linux)
ifeq ($(LINK_RELEASE_STATICALLY), 0)
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
	$(CC) $(LINKOPT) $(LINKOPT_RELEASE) $(OBJECTS_RELEASE) $(POSTLINKOPT) $(POSTLINKOPT_RELEASE) -o $(TARGET_RELEASE)

$(OBJECTS_RELEASE): .obj/%$(OBJECT_SUFFIX_RELEASE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(CCOPT_RELEASE) -c $< -o $@

## Build rules for the debug target follow.

debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJECTS_DEBUG)
	$(CC) $(LINKOPT) $(LINKOPT_DEBUG) $(OBJECTS_DEBUG) $(POSTLINKOPT) $(POSTLINKOPT_DEBUG) -o $(TARGET_DEBUG)

$(OBJECTS_DEBUG): .obj/%$(OBJECT_SUFFIX_DEBUG).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(CCOPT_DEBUG) -c $< -o $@

## Build rules for the profile target follow.

profile: $(TARGET_PROFILE)

$(TARGET_PROFILE): $(OBJECTS_PROFILE)
	$(CC) $(LINKOPT) $(LINKOPT_PROFILE) $(OBJECTS_PROFILE) $(POSTLINKOPT) $(POSTLINKOPT_PROFILE) -o $(TARGET_PROFILE)

$(OBJECTS_PROFILE): .obj/%$(OBJECT_SUFFIX_PROFILE).o: %.cc
	@mkdir -p $$(dirname $@)
	$(CC) $(CCOPT) $(CCOPT_PROFILE) -c $< -o $@

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

## Note: If we just call gcc -MM on a source file that lives within a
## subdirectory, it will strip the directory part in the output. Hence
## the for loop with the sed call.

Makefile.depend: $(SOURCES) $(HEADERS)
	rm -f Makefile.temp
	for source in $(SOURCES) ; do \
	    $(DEPEND) $$source > Makefile.temp0; \
	    objfile=$${source%%.cc}.o; \
	    sed -i -e "s@^[^:]*:@$$objfile:@" Makefile.temp0; \
	    cat Makefile.temp0 >> Makefile.temp; \
	done
	rm -f Makefile.temp0 Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_RELEASE).o:\2@" Makefile.temp >> Makefile.temp0
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_DEBUG).o:\2@" Makefile.temp >> Makefile.temp0
	sed -e "s@\(.*\)\.o:\(.*\)@.obj/\1$(OBJECT_SUFFIX_PROFILE).o:\2@" Makefile.temp >> Makefile.temp0
	rm -f Makefile.temp
	sed -e "s@\(.*\)\.$(STATE_VAR_BYTES)\.\(.*\)@\1.1.\2@" Makefile.temp0 >> Makefile.depend
	sed -e "s@\(.*\)\.$(STATE_VAR_BYTES)\.\(.*\)@\1.2.\2@" Makefile.temp0 >> Makefile.depend
	sed -e "s@\(.*\)\.$(STATE_VAR_BYTES)\.\(.*\)@\1.4.\2@" Makefile.temp0 >> Makefile.depend
	rm -f Makefile.temp0

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include Makefile.depend
endif
endif

.PHONY: default all release debug profile clean distclean
