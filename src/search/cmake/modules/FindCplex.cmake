set(_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
if(DEFINED ENV{DOWNWARD_CPLEX_ROOT})
    set(CMAKE_FIND_ROOT_PATH $ENV{DOWNWARD_CPLEX_ROOT})
    set(_COIN_ROOT_OPTS ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
endif()

find_path(CPLEX_INCLUDES
    NAMES
    cplex.h
    PATHS
    /include/ilcplex
    ${_COIN_ROOT_OPTS}
)

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(CPLEX_LIB_PATH "/lib/x86_sles10_4.1/static_pic")
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(CPLEX_LIB_PATH "/lib/x86-64_sles10_4.1/static_pic")
else()
    message(WARNING "Bitwidth could not be detected, guessing location of CPLEX")
    set(CPLEX_LIB_PATH "/lib/x86_sles10_4.1/static_pic /lib/x86-64_sles10_4.1/static_pic")
endif()

find_library(CPLEX_LIBRARIES
    cplex
    PATHS
    ${CPLEX_LIB_PATH}
    ${_COIN_ROOT_OPTS}
)

set(CMAKE_CXX_LINK_FLAGS_CPLEX "-pthread")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cplex
    REQUIRED_VARS CPLEX_INCLUDES CPLEX_LIBRARIES
)

mark_as_advanced(CPLEX_INCLUDES CPLEX_LIBRARIES)
