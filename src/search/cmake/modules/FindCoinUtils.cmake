if (COINUTILS_INCLUDES AND COINUTILS_LIBRARIES)
  set(COINUTILS_FIND_QUIETLY TRUE)
endif (COINUTILS_INCLUDES AND COINUTILS_LIBRARIES)

if(LP_PREFIX_PATH)
#    message("-- Forcing the use of CoinUtils in non-standard location ${LP_PREFIX_PATH}")

    find_path(COINUTILS_INCLUDES
        NAMES
        coin/CoinPragma.hpp
        PATHS
        ${LP_PREFIX_PATH}/include
        NO_DEFAULT_PATH
    )


    find_library(COINUTILS_LIBRARIES    
        CoinUtils
        PATHS
        ${LP_PREFIX_PATH}/lib
        NO_DEFAULT_PATH
    )
    
else(LP_PREFIX_PATH)
    
    find_path(COINUTILS_INCLUDES
        NAMES
        coin/CoinPragma.hpp
        PATHS
        ${INCLUDE_INSTALL_DIR}
    )


    find_library(COINUTILS_LIBRARIES
        CoinUtils
        PATHS
        ${LIB_INSTALL_DIR}
    )

endif(LP_PREFIX_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CoinUtils DEFAULT_MSG
                                  COINUTILS_INCLUDES COINUTILS_LIBRARIES)

mark_as_advanced(COINUTILS_INCLUDES COINUTILS_LIBRARIES)

