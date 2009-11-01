HEADERS = abstraction.h operator_registry.h variable_order_finder.h \
          heuristic.h \
	  lm_cut_heuristic.h \
          cyclic_cg_heuristic.h cg_heuristic.h \
          additive_heuristic.h \
          fd_heuristic.h ff_heuristic.h relaxation_heuristic.h\
	  max_heuristic.h \
          goal_count_heuristic.h blind_search_heuristic.h \
          globals.h \
	  domain_transition_graph.h \
          operator.h state.h successor_generator.h causal_graph.h \
	  best_first_search.h axioms.h cache.h \
          search_engine.h open_list_buckets.h closed_list.h \
          timer.h \
	  search_space.h fh_open_list.h a_star_search.h general_eager_best_first_search.h \
	  landmarks/landmarks_graph.h landmarks/landmarks_graph_rpg_sasp.h \
	  landmarks/landmarks_count_heuristic.h landmarks/exploration.h \
	  landmarks/landmark_status_manager.h \
	  landmarks/landmarks_graph_rpg_exhaust.h landmarks/landmarks_graph_zhu_givan.h \
	  landmarks/util.h landmarks/lama_ff_synergy.h \
	  hm_heuristic.h

SPECIAL_HEADERS = landmarks/landmark_cost_assignment.h

SOURCES = planner.cc $(HEADERS:%.h=%.cc)
OBJECTS = $(SOURCES:%.cc=obj/%.o)
PROFILE_OBJECTS = $(SOURCES:%.cc=obj/%.profile.o)
RELEASE_OBJECTS = $(SOURCES:%.cc=obj/%.release.o)

SPECIAL_SOURCES = $(SPECIAL_HEADERS:%.h=%.cc)
SPECIAL_OBJECTS = $(SPECIAL_SOURCES:%.cc=obj/%.o)
SPECIAL_PROFILE_OBJECTS = $(SPECIAL_SOURCES:%.cc=obj/%.profile.o)
SPECIAL_RELEASE_OBJECTS = $(SPECIAL_SOURCES:%.cc=obj/%.release.o)

CC = g++
DEPEND = g++ -MM
CCOPT         = -m32 -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror
LINKOPT = -m32
RELEASEOPT = $(CCOPT)
RELEASEOPT += -O3 -DNDEBUG -fomit-frame-pointer

SPECIAL_CCOPT = -m32 -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror -Wno-unused
SPECIAL_RELEASEOPT = $(SPECIAL_CCOPT)
SPECIAL_RELEASEOPT += -O3 -DNDEBUG -fomit-frame-pointer
## We link statically to allow local compiles to work on the BFG cluster.
## NOTE: This precludes using exceptions; see man gcc on -static-libgcc.
#RELEASEOPT += -static -static-libgcc

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


ifeq ($(USE_LP_CLP),1) 

COIN_ROOT=$(HOME)/Software/Osi
COIN_INC_DIR=$(COIN_ROOT)/include/coin
COIN_LIB_DIR=$(COIN_ROOT)/lib
SOLVER=CLP

ifeq ($(SOLVER),CLP)
SOLVER_INC_DIR=$(COIN_INC_DIR)
SOLVER_LIB_DIR=$(COIN_LIB_DIR)
SOLVER_LIBS=$(COIN_LIB_DIR)/libOsiClp.so $(SOLVER_LIB_DIR)/libClp.so
endif

ifeq ($(SOLVER),OSL)
SOLVER_INC_DIR=$(OSL_INC_DIR)
SOLVER_LIB_DIR=$(OSL_LIB_DIR)
SOLVER_LIBS=$(COIN_LIB_DIR)/libOsiOsl.so $(SOLVER_LIB_DIR)/libosl.so
endif

ifeq ($(SOLVER),CPX)
SOLVER_INC_DIR=$(CPX_INC_DIR)
SOLVER_LIB_DIR=$(CPX_LIB_DIR)
SOLVER_LIBS=$(COIN_LIB_DIR)/libOsiCpx.so $(SOLVER_LIB_DIR)/libcplex.a /usr/lib/libpthread.so
endif

COIN_FLAGS = -I$(COIN_INC_DIR) -I$(SOLVER_INC_DIR) -DCOIN_USE_$(SOLVER) -DUSE_LP 
COIN_LINK_FLAGS = $(COIN_LIB_DIR)/libOsi.so $(COIN_LIB_DIR)/libCoinUtils.so $(SOLVER_LIBS)

CCOPT += $(COIN_FLAGS)
LINKOPT += $(COIN_LINK_FLAGS)
RELEASELINKOPT = $(COIN_LINK_FLAGS)
endif

all: search

search: $(OBJECTS) $(SPECIAL_OBJECTS)
	$(CC) $(LINKOPT) $(OBJECTS) $(SPECIAL_OBJECTS) -o search

profile-search: $(PROFILE_OBJECTS)
	$(CC) $(LINKOPT) $(PROFLINKOPT) $(PROFOPT) $(PROFILE_OBJECTS) \
          -o profile-search

release: release-search

release-search: $(RELEASE_OBJECTS) $(SPECIAL_RELEASE_OBJECTS)
	$(CC) $(RELEASEOPT) $(RELEASELINKOPT) $(RELEASE_OBJECTS) $(SPECIAL_RELEASE_OBJECTS)\
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
	

$(SPECIAL_OBJECTS): obj/%.o: %.cc
	$(CC) $(SPECIAL_CCOPT) -c $< -o $@

$(SPECIAL_PROFILE_OBJECTS): obj/%.profile.o: %.cc
	$(CC) $(SPECIAL_CCOPT) $(PROFOPT) -c $< -o $@

$(SPECIAL_RELEASE_OBJECTS): obj/%.release.o: %.cc
	$(CC) $(SPECIAL_RELEASEOPT) -c $< -o $@	

clean:
	rm -f $(OBJECTS) $(PROFILE_OBJECTS) $(RELEASE_OBJECTS) $(SPECIAL_OBJECTS) $(SPECIAL_PROFILE_OBJECTS) $(SPECIAL_RELEASE_OBJECTS) *~ *.pyc
	rm -f Makefile.depend profile-search gmon.out PROFILE core
	rm -f sas_plan

distclean: clean
	rm -f search release-search

## Note: If we just call gcc -MM on a source file that lives within a
## subdirectory, it will strip the directory part in the output. Hence
## the for loop with the sed call.

Makefile.depend: $(SOURCES) $(HEADERS) $(SPECIAL_SOURCES) $(SPECIAL_HEADERS)
	rm -f Makefile.temp
	for source in $(SOURCES) $(SPECIAL_SOURCES); do \
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
