cmake_minimum_required(VERSION 3.16)

if(NOT FAST_DOWNWARD_MAIN_CMAKELISTS_READ)
    message(
        FATAL_ERROR
        "Run cmake on the CMakeLists.txt in the 'src' directory, "
        "not the one in 'src/search'. Please delete CMakeCache.txt "
        "from the current directory and restart cmake.")
endif()


## == Project ==

project(downward LANGUAGES CXX)

option(
  USE_GLIBCXX_DEBUG
  "Enable the libstdc++ debug mode that does additional safety checks. (On Linux \
systems, g++ and clang++ usually use libstdc++ for the C++ library.) The checks \
come at a significant performance cost and should only be enabled in debug mode. \
Enabling them makes the binary incompatible with libraries that are not compiled \
with this flag, which can lead to hard-to-debug errors."
  FALSE)

add_executable(downward planner.cc)

include("${CMAKE_CURRENT_SOURCE_DIR}/DownwardFiles.cmake")

## == Libraries ==

# TODO the next two libraries should be dependencies of the (interface) library
# that actually uses the corresponding methods (probably the one containing system.h?).

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

if(USE_LP)
    find_package(Cplex)
    if(CPLEX_FOUND)
        add_library(downward_cplex_interface INTERFACE)
        target_compile_definitions(downward_cplex_interface INTERFACE HAS_CPLEX)
        target_include_directories(downward_cplex_interface INTERFACE ${CPLEX_INCLUDE_DIRS})
        target_link_libraries(downward_cplex_interface INTERFACE ${CPLEX_LIBRARIES})
        if(CPLEX_RUNTIME_LIBRARY)
            add_custom_target(copy-cplex ALL)
            add_custom_command(TARGET copy-cplex POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CPLEX_RUNTIME_LIBRARY}
                $<TARGET_FILE_DIR:downward>
            )
        endif()
        target_sources(downward_cplex_interface INTERFACE lp/cplex_solver_interface.h lp/cplex_solver_interface.cc)
        target_link_libraries(downward_LP_SOLVER INTERFACE downward_cplex_interface)
    endif()

    find_package(SoPlex)
    if (SOPLEX_FOUND)
        add_library(downward_soplex_interface INTERFACE)
        target_compile_definitions(downward_soplex_interface INTERFACE HAS_SOPLEX)
        target_include_directories(downward_soplex_interface INTERFACE ${SOPLEX_INCLUDE_DIRS})
        target_link_libraries(downward_soplex_interface INTERFACE ${SOPLEX_LIBRARIES})
        find_package(ZLIB REQUIRED)
        target_include_directories(downward_soplex_interface INTERFACE ${ZLIB_INCLUDE_DIRS})
        target_link_libraries(downward_soplex_interface INTERFACE ${ZLIB_LIBRARIES})
        find_library(GMP_LIBRARY gmp REQUIRED)
        target_link_libraries(downward_soplex_interface INTERFACE ${GMP_LIBRARY})
        target_sources(downward_soplex_interface INTERFACE lp/soplex_solver_interface.h lp/soplex_solver_interface.cc)
        target_link_libraries(downward_LP_SOLVER INTERFACE downward_soplex_interface)
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
