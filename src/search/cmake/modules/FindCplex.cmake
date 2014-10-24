set(_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
if(NOT "$ENV{DOWNWARD_CPLEX_ROOT}" STREQUAL "")
    set(CMAKE_FIND_ROOT_PATH $ENV{DOWNWARD_CPLEX_ROOT})
    set(_COIN_ROOT_OPTS ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
endif(NOT "$ENV{DOWNWARD_CPLEX_ROOT}" STREQUAL "")

find_path(CPLEX_INCLUDES
    NAMES
    cplex.h
    PATHS
    /include/ilcplex
    ${_COIN_ROOT_OPTS}
)

find_library(CPLEX_LIBRARIES
    cplex
    PATHS
    /lib/x86_sles10_4.1/static_pic
    ${_COIN_ROOT_OPTS}
)

set(CMAKE_CXX_LINK_FLAGS_CPLEX "-pthread")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cplex
    REQUIRED_VARS CPLEX_INCLUDES CPLEX_LIBRARIES
)

mark_as_advanced(CPLEX_INCLUDES CPLEX_LIBRARIES)
