# Find Gurobi and export the target gurobi::gurobi
#
# Usage:
#   find_package(Gurobi)
#   target_link_libraries(<target> PRIVATE gurobi::gurobi)
#
# Hints:
#   -DGurobi_ROOT=...
#   -Dgurobi_DIR=...
#   env GUROBI_HOME=... (preferred)

set(HINT_PATHS ${Gurobi_ROOT} ${gurobi_DIR} $ENV{GUROBI_HOME})

find_path(GUROBI_INCLUDE_DIR
    NAMES gurobi_c.h gurobi_c++.h
    HINTS ${HINT_PATHS}
    PATH_SUFFIXES include
)

# For linux.
find_library(GUROBI_LIBRARY
    NAMES
        gurobi130   # Gurobi 13.0
	gurobi110
    HINTS ${HINT_PATHS}
    PATH_SUFFIXES lib 
)

# Check if everything was found and set Gurobi_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Gurobi
    REQUIRED_VARS GUROBI_INCLUDE_DIR GUROBI_LIBRARY
)

if(Gurobi_FOUND AND NOT TARGET gurobi::gurobi)
    add_library(gurobi::gurobi UNKNOWN IMPORTED)
    set_target_properties(gurobi::gurobi PROPERTIES
        IMPORTED_LOCATION "${GUROBI_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GUROBI_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(GUROBI_INCLUDE_DIR GUROBI_LIBRARY)
