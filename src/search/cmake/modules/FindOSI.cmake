# include(FindLibraryWithDebug)

if (OSI_INCLUDES AND OSI_LIBRARIES)
  set(OSI_FIND_QUIETLY TRUE)
endif (OSI_INCLUDES AND OSI_LIBRARIES)

if(LP_PREFIX_PATH)
#    message("-- Forcing the use of OSI in non-standard location ${LP_PREFIX_PATH}")
  
    
    find_path(OSI_INCLUDES
        coin/config_osi.h
        PATHS
        ${LP_PREFIX_PATH}/include
        NO_DEFAULT_PATH
    )


    find_library(OSI_LIBRARIES
        Osi
        PATHS
        ${LP_PREFIX_PATH}/lib
        NO_DEFAULT_PATH
    )


    find_library(OSICLP_LIBRARIES
        OsiClp
        PATHS
        ${LP_PREFIX_PATH}/lib
        NO_DEFAULT_PATH
    )
        
else(LP_PREFIX_PATH)

    find_path(OSI_INCLUDES
        NAMES
        coin/config_osi.h
        PATHS
        ${INCLUDE_INSTALL_DIR}
    )


    find_library(OSI_LIBRARIES
        Osi
        PATHS
        ${LIB_INSTALL_DIR}
    )


    find_library(OSICLP_LIBRARIES
        OsiClp
        PATHS
        ${LIB_INSTALL_DIR}
    )

endif(LP_PREFIX_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSI DEFAULT_MSG
                                  OSI_INCLUDES OSI_LIBRARIES)

mark_as_advanced(OSI_INCLUDES OSI_LIBRARIES)
