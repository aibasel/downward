find_path(COINUTILS_INCLUDES
    NAMES
    CoinPragma.hpp
    PATHS
    ${DOWNWARD_COIN_ROOT}
    $ENV{DOWNWARD_COIN_ROOT}
    PATH_SUFFIXES
    include
    include/coin
)

find_library(COINUTILS_LIBRARIES
    NAMES
    CoinUtils
    libCoinUtils
    PATHS
    ${DOWNWARD_COIN_ROOT}
    $ENV{DOWNWARD_COIN_ROOT}
    PATH_SUFFIXES
    lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CoinUtils
    REQUIRED_VARS COINUTILS_INCLUDES COINUTILS_LIBRARIES
)

mark_as_advanced(COINUTILS_INCLUDES COINUTILS_LIBRARIES)
