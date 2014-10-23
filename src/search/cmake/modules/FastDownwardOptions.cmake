## Variables prefixed with "DOWNWARD_" are used to configure the
## build. They can be changed as cmake arguments,
## e.g. -DDOWNWARD_BITWIDTH=64 or set with the ccmake GUI.

set(DOWNWARD_BITWIDTH
    "32"
    CACHE STRING
    "By default, build in 32-bit mode. Set to '64' to build in \
     64-bit mode and to 'native' to use the native bitness of the OS.")
if (${DOWNWARD_BITWIDTH} STREQUAL "32")
    set(BITWIDTHOPT, "-m32")
elseif (${DOWNWARD_BITWIDTH} STREQUAL "64")
    set(BITWIDTHOPT, "-m64")
elseif (NOT ${DOWNWARD_BITWIDTH} STREQUAL, "native")
    message( FATAL_ERROR "Bad value for DOWNWARD_BITWIDTH." )
endif

option(DOWNWARD_USE_LP
       "Enable linear programming stuff."
       NO)

option(DOWNWARD_LINK_RELEASE_STATICALLY
       "Set DOWNWARD_LINK_RELEASE_STATICALLY to 0 or 1 (default) to \
        disable/enable static linking of the executable in release mode. \
        On OS X, this is unsupported and will be silently disabled."
       YES)
if (${APPLE})
    set(DOWNWARD_LINK_RELEASE_STATICALLY NO)
endif


set(DOWNWARD_OS
    "auto"
    CACHE STRING
    "On a supported operating system, there should be no need to override \
     the DOWNWARD_OS setting. If the provided code does not work even \
     though your operating system is a supported one, please report this \
     as a bug.")

## Hint paths for LP libraries. These options default to environment
## variables with the same name, if they exist. If the environment
## variables are changed, the cmake cache is not automatically updated
## and should be updated manually or recreated.

set(DOWNWARD_COIN_ROOT
    $ENV{DOWNWARD_COIN_ROOT}
    CACHE STRING
    "Fast Downward uses the Open Solver Interface (OSI) by the \
     COIN-OR project for access to LP solvers. Set DOWNWARD_COIN_ROOT \
     to a path where OSI is installed.")
if (NOT DOWNWARD_COIN_ROOT)
    set(DOWNWARD_COIN_ROOT "/opt/coin")
endif

set(DOWNWARD_CLP_INCDIR
    $ENV{DOWNWARD_CLP_INCDIR}
    CACHE STRING
    "Set to the path of the CLP's include directory to use it through OSI.")
if (NOT DOWNWARD_CLP_INCDIR)
    set(DOWNWARD_CLP_INCDIR
        "${DOWNWARD_COIN_ROOT}/include/coin")
endif

set(DOWNWARD_CLP_LIBDIR
    $ENV{DOWNWARD_CLP_LIBDIR}
    CACHE STRING
    "Set to the path of the CLP's lib directory to use it through OSI.")
if (NOT DOWNWARD_CLP_LIBDIR)
    set(DOWNWARD_CLP_LIBDIR
        "${DOWNWARD_COIN_ROOT}/lib")
endif

set(DOWNWARD_CPLEX_INCDIR
    $ENV{DOWNWARD_CPLEX_INCDIR}
    CACHE STRING
    "Set to the path of the CPLEX's include directory to use it through OSI.")
if (NOT DOWNWARD_CPLEX_INCDIR)
    set(DOWNWARD_CPLEX_INCDIR
        "/opt/ibm/ILOG/CPLEX_Studio1251/cplex/include/ilcplex")
endif

set(DOWNWARD_CPLEX_LIBDIR
    $ENV{DOWNWARD_CPLEX_LIBDIR}
    CACHE STRING
    "Set to the path of the CPLEX's lib directory to use it through OSI.")
if (NOT DOWNWARD_CPLEX_LIBDIR)
    set(DOWNWARD_CPLEX_LIBDIR
        "/opt/ibm/ILOG/CPLEX_Studio1251/cplex/lib/x86_sles10_4.1/static_pic")
endif

set(DOWNWARD_GUROBI_INCDIR
    $ENV{DOWNWARD_GUROBI_INCDIR}
    CACHE STRING
    "Set to the path of the Gurobi's include directory to use it through OSI.")

set(DOWNWARD_GUROBI_LIBDIR
    $ENV{DOWNWARD_GUROBI_LIBDIR}
    CACHE STRING
    "Set to the path of the Gurobi's lib directory to use it through OSI.")
if (NOT DOWNWARD_GUROBI_LIBDIR)
    set(DOWNWARD_GUROBI_LIBDIR "")
endif

