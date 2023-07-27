cmake_minimum_required(VERSION 3.16)

if(NOT FAST_DOWNWARD_MAIN_CMAKELISTS_READ)
    message(
        FATAL_ERROR
        "Run cmake on the CMakeLists.txt in the 'src' directory, "
        "not the one in 'src/search'. Please delete CMakeCache.txt "
        "from the current directory and restart cmake.")
endif()


## == Project ==

project(downward)

option(
  USE_GLIBCXX_DEBUG
  "Enable the libstdc++ debug mode that does additional safety checks. (On Linux \
systems, g++ and clang++ usually use libstdc++ for the C++ library.) The checks \
come at a significant performance cost and should only be enabled in debug mode. \
Enabling them makes the binary incompatible with libraries that are not compiled \
with this flag, which can lead to hard-to-debug errors."
  FALSE)

# Collect source files needed for the active plugins.
include("${CMAKE_CURRENT_SOURCE_DIR}/DownwardFiles.cmake")
add_executable(downward ${PLANNER_SOURCES})

target_link_libraries(downward PUBLIC fd_interface_library)
get_target_property(bin_dir fd_interface_library RUNTIME_OUTPUT_DIRECTORY)
set_target_properties(downward PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${bin_dir})

## == Libraries ==

# On Linux, find the rt library for clock_gettime().
if(UNIX AND NOT APPLE)
    target_link_libraries(downward PRIVATE rt)
endif()

# On Windows, find the psapi library for determining peak memory.
if(WIN32)
    cmake_policy(SET CMP0074 NEW)
    target_link_libraries(downward PRIVATE psapi)
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
        target_link_libraries(downward PRIVATE ${CPLEX_LIBRARIES})
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
        target_link_libraries(downward PRIVATE ${SOPLEX_LIBRARIES})
        find_package(ZLIB REQUIRED)
        if(ZLIB_FOUND)
            include_directories(${ZLIB_INCLUDE_DIRS})
            target_link_libraries(downward PRIVATE ${ZLIB_LIBRARIES})
        endif()
        find_library(GMP_LIBRARY gmp REQUIRED)
        if(GMP_LIBRARY)
            target_link_libraries(downward PRIVATE ${GMP_LIBRARY})
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
