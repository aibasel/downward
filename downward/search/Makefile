HEADERS = \
          abstraction.h \
          additive_heuristic.h \
          axioms.h \
          best_first_search.h \
          blind_search_heuristic.h \
          cache.h \
          causal_graph.h \
          cg_heuristic.h \
          closed_list.h \
          cyclic_cg_heuristic.h \
          domain_transition_graph.h \
          enforced_hill_climbing_search.h \
          ff_heuristic.h \
          general_eager_best_first_search.h \
          general_lazy_best_first_search.h \
          globals.h \
          goal_count_heuristic.h \
          heuristic.h \
          hm_heuristic.h \
          lm_cut_heuristic.h \
          mas_heuristic.h \
          max_heuristic.h \
          operator.h \
          operator_registry.h \
          option_parser.h \
          pref_evaluator.h \
          relaxation_heuristic.h \
          search_engine.h \
          search_node_info.h \
          search_progress.h \
          search_space.h \
          state.h \
          successor_generator.h \
          sum_evaluator.h g_evaluator.h \
          timer.h \
          utilities.h \
          variable_order_finder.h \
          weighted_evaluator.h \
          \
          open_lists/alternation_open_list.h \
          open_lists/open_list_buckets.h \
          open_lists/pareto_open_list.h \
          open_lists/standard_scalar_open_list.h \
          open_lists/tiebreaking_open_list.h \
          \
          landmarks/exploration.h \
          landmarks/lama_ff_synergy.h \
          landmarks/landmark_cost_assignment.h \
          landmarks/landmark_status_manager.h \
          landmarks/landmarks_count_heuristic.h \
          landmarks/landmarks_graph.h \
          landmarks/landmarks_graph_rpg_exhaust.h \
          landmarks/landmarks_graph_rpg_sasp.h \
          landmarks/landmarks_graph_rpg_search.h \
          landmarks/landmarks_graph_zhu_givan.h \
          landmarks/util.h \
          \
          learning/AODE.h \
          learning/classifier.h \
          learning/composite_feature_extractor.h \
          learning/feature_extractor.h \
          learning/landmarks_feature_extractor.h \
          learning/maximum_heuristic.h \
          learning/naive_bayes_classifier.h \
          learning/node_info_feature_extractor.h \
          learning/g_feature_extractor.h \
          learning/PDB_state_space_sample.h \
          learning/probe_state_space_sample.h \
          learning/selective_max_heuristic.h \
          learning/state_space_sample.h \
          learning/state_vars_feature_extractor.h

SOURCES = planner.cc $(HEADERS:%.h=%.cc)
OBJECTS = $(SOURCES:%.cc=obj/%.o)
PROFILE_OBJECTS = $(SOURCES:%.cc=obj/%.profile.o)
RELEASE_OBJECTS = $(SOURCES:%.cc=obj/%.release.o)

CC = g++
DEPEND = g++ -MM
CCOPT         = -m32 -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror
LINKOPT = -m32
RELEASEOPT = $(CCOPT)
RELEASEOPT += -O3 -DNDEBUG -fomit-frame-pointer

## We link statically to allow local compiles to work on the BFG cluster.
## NOTE: This precludes using exceptions; see man gcc on -static-libgcc.
RELEASEOPT += -static -static-libgcc

## debug, don't optimize
CCOPT += -g
LINKOPT += -g

## debug, optimize
# CCOPT += -O3 -g
# LINKOPT += -g

## no debug, optimize a lot
# CCOPT += -O3 -DNDEBUG

PROFOPT = -pg -O3
# PROFLINKOPT = -lc_p
PROFARGS = oa200000 < profile-input.pre

USE_COIN_CLP=0

ifeq ($(USE_LP_CLP),1) 
COIN_ROOT = lp/coin

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


## We want to link the COIN libraries statically and link other
## libraries dynamically, unless we are in release mode. We accomplish
## this by using -Wl,-Bstatic before the COIN libs and -Wl,-Bdynamic
## afterwards (unless in release mode). See
## http://ubuntuforums.org/showthread.php?t=491455

COIN_FLAGS = -I $(COIN_ROOT)/include/coin -D USE_LP -D COIN_USE_CLP
CCOPT += $(COIN_FLAGS)

COIN_LINK_FLAGS = -L $(COIN_ROOT)/lib -Wl,-Bstatic $(COIN_LIBS:%=-l %)
RELEASELINKOPT_END += $(COIN_LINK_FLAGS)
LINKOPT_END += $(COIN_LINK_FLAGS) -Wl,-Bdynamic
endif

all: search

search: $(OBJECTS)
	$(CC) $(LINKOPT) $(OBJECTS) $(LINKOPT_END) -o search

profile-search: $(PROFILE_OBJECTS)
	$(CC) $(LINKOPT) $(PROFLINKOPT) $(PROFOPT) $(PROFILE_OBJECTS) $(LINKOPT_END)\
          -o profile-search

release: release-search

release-search: $(RELEASE_OBJECTS) 
	$(CC) $(RELEASEOPT) $(RELEASELINKOPT) $(RELEASE_OBJECTS) $(RELEASELINKOPT_END)\
	  -o release-search

PROFILE: profile-search
	./profile-search $(PROFARGS)
	gprof profile-search | cleanup-profile > PROFILE

$(OBJECTS): obj/%.o: %.cc
	$(CC) $(CCOPT) -c $< -o $@

$(PROFILE_OBJECTS): obj/%.profile.o: %.cc
	$(CC) $(CCOPT) $(PROFOPT) -c $< -o $@

$(RELEASE_OBJECTS): obj/%.release.o: %.cc
	$(CC) $(RELEASEOPT) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(PROFILE_OBJECTS) $(RELEASE_OBJECTS) *~ *.pyc
	rm -f Makefile.depend profile-search gmon.out PROFILE core
	rm -f sas_plan

distclean: clean
	rm -f search release-search

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
	rm -f Makefile.temp0
	sed -e "s@\(.*\)\.o:\(.*\)@obj/\1.o:\2@" Makefile.temp > Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@obj/\1.release.o:\2@" Makefile.temp >> Makefile.depend
	sed -e "s@\(.*\)\.o:\(.*\)@obj/\1.profile.o:\2@" Makefile.temp >> Makefile.depend
	rm -f Makefile.temp

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include Makefile.depend
endif
endif

.PHONY: release clean distclean
