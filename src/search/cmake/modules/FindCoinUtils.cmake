set(_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
if(DEFINED ENV{DOWNWARD_COIN_ROOT})
    set(CMAKE_FIND_ROOT_PATH $ENV{DOWNWARD_COIN_ROOT})
    set(_COIN_ROOT_OPTS ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
endif()

find_path(COINUTILS_INCLUDES
    NAMES
    CoinPragma.hpp
    PATHS
    /include/coin
    ${_COIN_ROOT_OPTS}
)

find_library(COINUTILS_LIBRARIES
    CoinUtils
    PATHS
    /lib
    ${_COIN_ROOT_OPTS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CoinUtils
    REQUIRED_VARS COINUTILS_INCLUDES COINUTILS_LIBRARIES
)

mark_as_advanced(COINUTILS_INCLUDES COINUTILS_LIBRARIES)
