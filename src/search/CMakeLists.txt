cmake_minimum_required(VERSION 2.8.3)
# For Windows we require CMake 3.12, but this is currently not
# available for Ubuntu 18.04.

if(NOT FAST_DOWNWARD_MAIN_CMAKELISTS_READ)
    message(
        FATAL_ERROR
        "Run cmake on the CMakeLists.txt in the 'src' directory, "
        "not the one in 'src/search'. Please delete CMakeCache.txt "
        "from the current directory and restart cmake.")
endif()


## == Project ==

project(downward)

# TODO: use multi-line strings to break up the long string when we switch to CMake 3.
# https://stackoverflow.com/questions/7637539/how-to-split-strings-across-multiple-lines-in-cmake
option(
  USE_GLIBCXX_DEBUG
  "Enable the libstdc++ debug mode that does additional safety checks. (On Linux systems, g++ and clang++ usually use libstdc++ for the C++ library.) The checks come at a significant performance cost and should only be enabled in debug mode. Enabling them makes the binary incompatible with libraries that are not compiled with this flag, which can lead to hard-to-debug errors."
  FALSE)

fast_downward_set_compiler_flags()
fast_downward_set_linker_flags()

# Collect source files needed for the active plugins.
include("${CMAKE_CURRENT_SOURCE_DIR}/DownwardFiles.cmake")
add_executable(downward ${PLANNER_SOURCES})

## == Libraries ==

# On Linux, find the rt library for clock_gettime().
if(UNIX AND NOT APPLE)
    target_link_libraries(downward rt)
endif()

# On Windows, find the psapi library for determining peak memory.
if(WIN32)
    cmake_policy(SET CMP0074 NEW)
    target_link_libraries(downward psapi)
endif()

# If any enabled plugin requires an LP solver, compile with all
# available LP solvers. If no solvers are installed, the planner will
# still compile, but using heuristics that depend on an LP solver will
# cause an error. This behavior can be overwritten by setting the
# option USE_LP to false.
option(
  USE_LP
  "Compile with support for all LP solvers installed on this system."
  TRUE)

if(PLUGIN_LP_SOLVER_ENABLED AND USE_LP)
    add_definitions("-D USE_LP")

    find_package(Cplex)
    if(CPLEX_FOUND)
        add_definitions("-D HAS_CPLEX")
        include_directories(${CPLEX_INCLUDE_DIRS})
        target_link_libraries(downward ${CPLEX_LIBRARIES})
        if(CPLEX_RUNTIME_LIBRARY)
            add_custom_command(TARGET downward POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CPLEX_RUNTIME_LIBRARY}
                $<TARGET_FILE_DIR:downward>
            )
        endif()
    endif()

    find_package(SoPlex)
    if (SOPLEX_FOUND)
        add_definitions("-D HAS_SOPLEX")
        include_directories(${SOPLEX_INCLUDE_DIRS})
        target_link_libraries(downward ${SOPLEX_LIBRARIES})
        find_package(ZLIB REQUIRED)
        if(ZLIB_FOUND)
            include_directories(${ZLIB_INCLUDE_DIRS})
            target_link_libraries(downward ${ZLIB_LIBRARIES})
        endif()
        find_library(GMP_LIBRARY gmp REQUIRED)
        if(GMP_LIBRARY)
            target_link_libraries(downward ${GMP_LIBRARY})
        endif()
    endif()

    if(CPLEX_FOUND OR SOPLEX_FOUND)
        if(USE_GLIBCXX_DEBUG)
            message(
                FATAL_ERROR
                "To prevent incompatibilities, the option USE_GLIBCXX_DEBUG is "
                "not supported when an LP solver is used. See issue982 for details.")
        endif()
    endif()

endif()
