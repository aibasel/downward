DOWNWARD_BITWIDTH=32

HEADERS = \
          ../../../src/search/axioms.h \
          ../../../src/search/causal_graph.h \
          ../../../src/search/combining_evaluator.h \
          ../../../src/search/countdown_timer.h \
          ../../../src/search/domain_transition_graph.h \
          ../../../src/search/eager_search.h \
          ../../../src/search/enforced_hill_climbing_search.h \
          ../../../src/search/equivalence_relation.h \
          ../../../src/search/exact_timer.h \
          ../../../src/search/g_evaluator.h \
          ../../../src/search/global_operator.h \
          ../../../src/search/global_state.h \
          ../../../src/search/globals.h \
          ../../../src/search/global_task_interface.h \
          ../../../src/search/heuristic.h \
          ../../../src/search/int_packer.h \
          ../../../src/search/ipc_max_heuristic.h \
          ../../../src/search/iterated_search.h \
          ../../../src/search/lazy_search.h \
          ../../../src/search/max_evaluator.h \
          ../../../src/search/operator_cost.h \
          ../../../src/search/option_parser.h \
          ../../../src/search/option_parser_util.h \
          ../../../src/search/segmented_vector.h \
          ../../../src/search/per_state_information.h \
          ../../../src/search/pref_evaluator.h \
          ../../../src/search/relaxation_heuristic.h \
          ../../../src/search/rng.h \
          ../../../src/search/search_engine.h \
          ../../../src/search/search_node_info.h \
          ../../../src/search/search_progress.h \
          ../../../src/search/search_space.h \
          ../../../src/search/state_id.h \
          ../../../src/search/state_registry.h \
          ../../../src/search/successor_generator.h \
          ../../../src/search/sum_evaluator.h \
          ../../../src/search/timer.h \
          ../../../src/search/utilities.h \
          ../../../src/search/variable_order_finder.h \
          ../../../src/search/weighted_evaluator.h \
          \
          ../../../src/search/open_lists/alternation_open_list.h \
          ../../../src/search/open_lists/open_list_buckets.h \
          ../../../src/search/open_lists/pareto_open_list.h \
          ../../../src/search/open_lists/standard_scalar_open_list.h \
          ../../../src/search/open_lists/tiebreaking_open_list.h \
          \
          ../../../src/search/lp_solver_interface.h \

SOURCES = main.cc $(HEADERS:%.h=%.cc)
TARGET = benchmark

default: release

OBJECT_SUFFIX_RELEASE = .release
TARGET_SUFFIX_RELEASE =
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
    $(error Bad value for DOWNWARD_BITWIDTH)
endif

CXXFLAGS =
CXXFLAGS += -g
CXXFLAGS += $(BITWIDTHOPT)
CXXFLAGS += -I../../../src/search/ext
# Note: we write "-std=c++0x" rather than "-std=c++11" to support gcc 4.4.
CXXFLAGS += -std=c++0x -Wall -Wextra -pedantic -Wno-deprecated -Werror

LDFLAGS =
LDFLAGS += $(BITWIDTHOPT)
LDFLAGS += -g

POSTLINKOPT =

CXXFLAGS_RELEASE  = -O3 -DNDEBUG -fomit-frame-pointer
CXXFLAGS_DEBUG    = -O3
CXXFLAGS_PROFILE  = -O3 -pg

LDFLAGS_RELEASE  =
LDFLAGS_DEBUG    =
LDFLAGS_PROFILE  = -pg

POSTLINKOPT_RELEASE =
POSTLINKOPT_DEBUG   =
POSTLINKOPT_PROFILE =

LDFLAGS_RELEASE += -static -static-libgcc

POSTLINKOPT_RELEASE += -Wl,-Bstatic -lrt
POSTLINKOPT_DEBUG  += -lrt
POSTLINKOPT_PROFILE += -lrt

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
