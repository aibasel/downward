
find_path(CPLEX_INCLUDES
    NAMES
    cplex.h
    PATHS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    include/ilcplex
)

#TODO: version and path detection
if(UNIX)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(CPLEX_LIB_PATH "/lib/x86_sles10_4.1/static_pic")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(CPLEX_LIB_PATH "/lib/x86-64_sles10_4.1/static_pic")
    else()
        message(WARNING "Bitwidth could not be detected, guessing location of CPLEX")
        set(CPLEX_LIB_PATH "/lib/x86_sles10_4.1/static_pic /lib/x86-64_sles10_4.1/static_pic")
    endif()
endif()

if(MSVC)
    set(CPLEX_LIB_PATH "${DOWNWARD_CPLEX_ROOT}/lib/x86_windows_vs2013/stat_mda")
endif()

find_library(CPLEX_LIBRARIES
    NAMES
    cplex
    cplex1262
    PATHS
    ${CPLEX_LIB_PATH}
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    lib

)

if(CPLEX_LIBRARIES)
    find_package(Threads REQUIRED)

    set(CPLEX_LIBRARIES ${CPLEX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cplex
    REQUIRED_VARS CPLEX_INCLUDES CPLEX_LIBRARIES
)

mark_as_advanced(CPLEX_INCLUDES CPLEX_LIBRARIES)
