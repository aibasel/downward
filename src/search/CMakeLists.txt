### TODO documentation is out of date

###   * Usage: cmake . ; make [-j2]
###   * The first command creates and stores options in the file CMakeCache.txt.
###     Changing the default values for the options here has no effect, you need
###     to change them in CMakeCache.txt or pass -DMY_OPTION to cmake.
###
### You probably won't need to change this system, unless you want to tweak it for a specific
### platform.  By default, four build targets are defined:
###
### * -DCMAKE_BUILD_TYPE=Debug          - no optimisation, full debugging information
### * -DCMAKE_BUILD_TYPE=Release        - -O3 optimisation, static binary, no debugging information
### * -DCMAKE_BUILD_TYPE=RelWithDebInfo - -O3 optimisation, static binary, but with debugging information
### * -DCMAKE_BUILD_TYPE=Profile        - -O3 optimisation, profile information linked in
###
### We overwrite the default configuration to include "-g", allow cross
### compilation and switch to pedantic error reporting.


# Version 2.6 introduces functions.
cmake_minimum_required(VERSION 2.6)



## == Default options ==

# Default to Release build, unless specified otherwise
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile")
if(NOT CMAKE_BUILD_TYPE)
    message("Defaulting to release build.")
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING
        "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
        FORCE)
endif()

# Default tool chain forces a 32 bit compilation
# disable tool chain file for now
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message("Defaulting to 32 bit compilation.")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/toolchain-32bit.cmake")
endif()

# Path containing custom CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)



## == Project ==

set(TARGET "downward")

project(${TARGET})



## == Target and source files ==

# Output binary to src folder and use build type as suffix.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
string(TOLOWER "${CMAKE_BUILD_TYPE}" LOWER_CASE_BUILD_TYPE)
set(TARGET "${TARGET}-${LOWER_CASE_BUILD_TYPE}")

# Collect source files needed for the active plugins.
include("FastDownwardPlugins")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext)

add_executable(${TARGET}
    planner.cc
    ${search_main_SRCS}
    ${search_plugin_SRCS}
)



## == Build flags ==

if(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror")

    if(APPLE)
        ## If you have Mac OS, you may be getting tree.hh from /opt/local/include.
        ## TODO: we do ship tree.hh so why add another external include
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/local/include/")
    endif()

    set(CMAKE_CXX_LINK_FLAGS "")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${BITWIDTHOPT}")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -g")


    ## Configuration-specific flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "-O3")
    set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg")

    set(CMAKE_CXX_LINK_FLAGS_PROFILE "-pg")
elseif(MSVC)
    # enable exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc")
    set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_CXX_FLAGS_DEBUG})

    # disable warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4018 -wd4101 -wd4996")
    # -WX == -Werror
    # -Wall is also avalible but gives you way to many warnings
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
