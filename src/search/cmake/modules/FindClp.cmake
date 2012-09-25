# include(FindLibraryWithDebug)

if (CLP_INCLUDES AND CLP_LIBRARIES)
  set(CLP_FIND_QUIETLY TRUE)
endif (CLP_INCLUDES AND CLP_LIBRARIES)

if(LP_PREFIX_PATH)
    
#    message("-- Forcing the use of Clp in non-standard location ${LP_PREFIX_PATH}")
    
    find_path(CLP_INCLUDES
        NAMES
        coin/ClpSimplex.hpp
        PATHS
        ${LP_PREFIX_PATH}/include
        NO_DEFAULT_PATH
    )

    find_library(CLP_LIBRARIES
        Clp
        PATHS
        ${LP_PREFIX_PATH}/lib
        NO_DEFAULT_PATH
    )

  
else(LP_PREFIX_PATH)
    
    find_path(CLP_INCLUDES
        NAMES
        coin/ClpSimplex.hpp
        PATHS
        ${INCLUDE_INSTALL_DIR}
    )

    find_library(CLP_LIBRARIES
        Clp
        PATHS
        ${LIB_INSTALL_DIR}
    )
    
endif(LP_PREFIX_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clp DEFAULT_MSG
                                  CLP_INCLUDES CLP_LIBRARIES)

mark_as_advanced(CLP_INCLUDES CLP_LIBRARIES)

