HEADERS = multi_bfs.h heuristic.h domain_transition_graph.h globals.h \
          operator.h state.h successor_generator.h causal_graph.h \
	  best_first_search.h ff_heuristic.h axioms.h \
          cache.h scheduler.h \
          iterative_search.h
# horn.h deadend.h
# enforced_hill_climbing.h

SOURCES = planner.cc $(HEADERS:%.h=%.cc)
OBJECTS = $(SOURCES:%.cc=%.o)
PROFILE_OBJECTS = $(SOURCES:%.cc=%.profile.o)

CC = g++
DEPEND = g++ -MM
CCOPT = -O3 -march=pentium3
# CCOPT += -DNDEBUG
LINKOPT =
PROFOPT = -pg
PROFLINKOPT = -lc_p

vpath %.o obj

search: $(OBJECTS)
	$(CC) $(LINKOPT) $(OBJECTS:%=obj/%) -o search

profile-search: $(PROFILE_OBJECTS)
	$(CC) $(LINKOPT) $(PROFLINKOPT) $(PROFOPT) $(PROFILE_OBJECTS:%=obj/%) \
          -o profile-search

PROFILE: profile-search
	profile-search < profile-problem.in
	gprof profile-search > PROFILE

$(OBJECTS): %.o: %.cc
	$(CC) $(CCOPT) -c $< -o obj/$@

$(PROFILE_OBJECTS): %.profile.o: %.cc
	$(CC) $(CCOPT) $(PROFOPT) -c $< -o obj/$@

clean:
	rm -f $(OBJECTS:%=obj/%) $(PROFILE_OBJECTS:%=obj/%) *~ *.pyc
	rm -f Makefile.depend profile-search gmon.out PROFILE

veryclean: clean
	rm -f search

Makefile.depend: $(SOURCES) $(HEADERS)
	$(DEPEND) $(SOURCES) > Makefile.temp
	sed -e "s/\(.*\)\.o:\(.*\)/\1.profile.o:\2/" Makefile.temp >> Makefile.temp2
	cat Makefile.temp Makefile.temp2 > Makefile.depend
	rm -f Makefile.temp Makefile.temp2

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),veryclean)
-include Makefile.depend
endif
endif

