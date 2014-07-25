## Variables prefixed with "DOWNWARD_" are used to configure the 
## build. They can be changed on the command line or set as 
## environment variables.

## By default, build in 32-bit mode. Use DOWNWARD_BITWIDTH=64
## to build in 64-bit mode and DOWNWARD_BITWIDTH=native to use
## the native bitness of the OS.
DOWNWARD_BITWIDTH ?= 32

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
          axiom.h \
          causal_graph.h \
          domain_transition_graph.h \
          helper_functions.h \
          max_dag.h \
          mutex_group.h \
          operator.h \
          scc.h \
          state.h \
          successor_generator.h \
          variable.h \

SOURCES = planner.cc $(HEADERS:%.h=%.cc)
TARGET = preprocess

default: release

ARGS_PROFILE =

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
    else ifeq ($(DOWNWARD_OS), auto)
        $(warning OS detection failed -- setting to Linux and hoping for the best!)
        DOWNWARD_OS=linux
    endif
endif

ifeq ($(DOWNWARD_OS), osx)
    ## Disable static linking on OS X.
    DOWNWARD_LINK_RELEASE_STATICALLY=0
endif


OBJECT_SUFFIX_RELEASE =
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


CXX     = g++
DEPEND = g++ -MM

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
CXXFLAGS += -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror

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
	rm -f output

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
