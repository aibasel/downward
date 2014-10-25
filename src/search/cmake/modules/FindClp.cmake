set(_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
if(DEFINED ENV{DOWNWARD_COIN_ROOT})
    set(CMAKE_FIND_ROOT_PATH $ENV{DOWNWARD_COIN_ROOT})
    set(_COIN_ROOT_OPTS ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
endif()

find_path(CLP_INCLUDES
    NAMES
    ClpSimplex.hpp
    PATHS
    /include/coin
    ${_COIN_ROOT_OPTS}
)

find_library(CLP_LIBRARIES
    Clp
    PATHS
    /lib
    ${_COIN_ROOT_OPTS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Clp
    REQUIRED_VARS CLP_INCLUDES CLP_LIBRARIES
)

mark_as_advanced(CLP_INCLUDES CLP_LIBRARIES)
