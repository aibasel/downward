###   * Usage:
###         mkdir build
###         cd build
###         cmake path/to/src/search
###         make [-j4]
###   * The call to cmake creates and stores options in the file CMakeCache.txt.
###     Changing the default values for the options here has no effect, you need
###     to change them in CMakeCache.txt, pass -DMY_OPTION to cmake, or use a
###     cmake GUI like ccmake to edit the cache.
###
### You probably won't need to change this system, unless you want to tweak it for a specific
### platform. By default, three build targets are defined:
###
### * -DCMAKE_BUILD_TYPE=Release        - -O3 optimisation, debugging symbols, assertions inactive
### * -DCMAKE_BUILD_TYPE=Debug          - -O3 optimisation, full debugging information, assertions active
### * -DCMAKE_BUILD_TYPE=Profile        - like Debug but with profile information linked in
###
### We overwrite the default configuration to include "-g", allow cross
### compilation and switch to pedantic error reporting.


# Version 2.6 introduces functions.
cmake_minimum_required(VERSION 2.6)



## == Default options ==

# Default to Release build, unless specified otherwise
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile")
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}"
    CACHE STRING "List of supported configurations.")
mark_as_advanced(CMAKE_CONFIGURATION_TYPES)

if(NOT CMAKE_BUILD_TYPE)
    message("Defaulting to release build.")
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING
        "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
        FORCE)
endif()

# Path containing custom CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Allow to compile a 64-bit version.
# Since compiling for 32-bit works differently on each platform, we let
# the users set up their own build environment and only check which one
# is used. Compiling a 64-bit version of the planner without explicitly
# settig ALLOW_64_BIT to true results in an error.
option(ALLOW_64_BIT "Allow to compile a 64-bit version." FALSE)

# On Unix, we explicitly force compilation to 32-bit unless ALLOW_64_BIT is set.
# Has to be done before defining the project.
if(UNIX AND NOT ALLOW_64_BIT)
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "c++ flags")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32" CACHE STRING "c flags")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32" CACHE STRING "linker flags")
endif()

# Use all available LP solvers by default. If no solvers are installed,
# this option can remain true. Setting this to false will compile without
# LP solvers even if some of them are installed.
option(USE_LP "Use all available LP solvers." TRUE)



## == Project ==

set(TARGET "downward")

project(${TARGET})



## == Validating options ==

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    if(ALLOW_64_BIT)
        message(WARNING "Bitwidth is 32-bit but ALLOW_64_BIT is set. "
        "Do not set ALLOW_64_BIT unless you are sure you want a 64-bit build. "
        "See http://www.fast-downward.org/PlannerUsage#A64bit for details.")
    else()
        message("Building for 32-bit.")
    endif()
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    if(ALLOW_64_BIT)
        message("Building for 64-bit.")
    else()
        message(FATAL_ERROR "You are compiling the planner for 64-bit, "
        "which is not recommended. "
        "Use -DALLOW_64_BIT=true if you need a 64-bit build. "
        "See http://www.fast-downward.org/PlannerUsage#A64bit for details.")
    endif()
else()
    message(FATAL_ERROR, "Could not determine bitwidth. We recommend to "
    "build the planner for 32-bit. "
    "See http://www.fast-downward.org/PlannerUsage#A64bit for details.")
endif()



## == Target and source files ==

# Output binary to src folder and use build type as suffix.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
string(TOLOWER "${CMAKE_BUILD_TYPE}" LOWER_CASE_BUILD_TYPE)
set(TARGET "${TARGET}-${LOWER_CASE_BUILD_TYPE}")

# Collect source files needed for the active plugins.
include("FastDownwardPlugins")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext)

add_executable(${TARGET} ${PLANNER_SOURCES})



## == Build flags ==

if(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag( "-std=c++11" CXX11_FOUND )
    if(CXX11_FOUND)
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        message(STATUS "${CMAKE_CXX_COMPILER} does not support C++11, please use a different compiler")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror")

    if(APPLE)
        ## If you have Mac OS, you may be getting tree.hh from /opt/local/include.
        ## TODO: we do ship tree.hh so why add another external include
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/local/include/")
    endif()

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")


    ## Configuration-specific flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "-O3")
    set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg")

    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg")
elseif(MSVC)
    # enable exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_CXX_FLAGS_DEBUG})

    # Use warning level 4 (/W4) and treat warnings as errors (/WX)
    # -Wall currently detects too many warnings outside of our code to be useful.
    string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

    # Disable warnings that currently trigger in the code until we fix them.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800") # forcing value to bool
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4512") # assignment operator could not be generated
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4706") # assignment within conditional expression (in tree.hh)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unreferenced formal parameter (in OSI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127") # conditional expression is constant (in tree.hh and in our code)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244") # conversion with possible loss of data
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4702") # unreachable code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4239") # nonstandard extension used
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996") # function call with parameters that may be unsafe

    # TODO: Configuration-specific flags
endif()



## == Libraries ==

# On Linux, find the rt library for the exact timer
if(UNIX AND NOT APPLE)
    target_link_libraries(${TARGET} rt)
endif()

# On Windows, find the psapi library for determining peak memory
if(WIN32)
    target_link_libraries(${TARGET} psapi)
endif()

if(USE_LP)
    find_package(OSI OPTIONAL_COMPONENTS Cpx Clp Grb)
    if(OSI_FOUND AND (OSI_Cpx_FOUND OR OSI_Clp_FOUND OR OSI_Grb_FOUND))
        foreach(SOLVER Cpx Clp Grb)
            if(OSI_${SOLVER}_FOUND)
                string(TOUPPER ${SOLVER} TMP_SOLVER_UPPER_CASE)
                mark_as_advanced(TMP_SOLVER_UPPER_CASE)
                add_definitions("-D COIN_HAS_${TMP_SOLVER_UPPER_CASE}")
                include_directories(${OSI_${SOLVER}_INCLUDE_DIRS})
                target_link_libraries(${TARGET} ${OSI_${SOLVER}_LIBRARIES})
            endif()
        endforeach()

        # Note that basic Osi libs must be added after (!) all osi solver libs.
        add_definitions("-D USE_LP")
        include_directories(${OSI_INCLUDE_DIRS})
        target_link_libraries(${TARGET} ${OSI_LIBRARIES})
    endif()

    if(CPLEX_RUNTIME_LIBRARY)
        file(COPY ${CPLEX_RUNTIME_LIBRARY} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
endif()
