# Usage:
#         cmake -S src -B builds/release
#         cmake --build builds/release
# The first call caches settings in the build directory and reads
# them from the cache on subsequent builds. If you want to change the
# settings of some options, do _not_ change the CMakeLists.txt files.
# Instead, create a new build directory, pass -DMY_OPTION=my_value to
# cmake. Alternatively, you can use a cmake GUI like ccmake to edit
# the cache.
#
# Two build targets are defined:
#
# * release (default)
#      -O3 optimisation, debugging symbols, assertions inactive
# * debug
#      -O3 optimisation, full debugging information, assertions active
#
# In all build targets, we overwrite the default configuration to
# include "-g", allow cross compilation and switch to pedantic error
# reporting.
#
# You can change the build target, by adding the parameter
#   -DCMAKE_BUILD_TYPE=type
# to the cmake call.

cmake_minimum_required(VERSION 3.16)

# Select a default compiler because CMake does not respect the PATH environment variable for some generators,
# e.g. used by existing compute servers in Basel.
# See https://stackoverflow.com/a/45934279 and https://issues.fast-downward.org/issue1031 for details.
find_program(CMAKE_C_COMPILER NAMES $ENV{CC} cc gcc PATHS ENV PATH NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} c++ g++ PATHS ENV PATH NO_DEFAULT_PATH)

# Path containing custom CMake modules
# TODO: I could not find anything other than modifying the CMAKE_* variable directly,
# but appending to the list sounds at least better than just setting the variable
# https://cmake.org/cmake/help/latest/variable/CMAKE_MODULE_PATH.html#variable:CMAKE_MODULE_PATH
# says the variable is intended to be set by the project, so I guess this is the intended way?
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
include(FastDownwardMacros)

project(fast-downward)
#TODO: is there a better place for this call?
define_interface_library()
fast_downward_report_bitwidth()
# Due to a bug in cmake, configuration types are only set up correctly on the second cmake run.
# This means that cmake has to be called twice for multi-config generators like Visual Studio.
fast_downward_set_configuration_types()

set(FAST_DOWNWARD_MAIN_CMAKELISTS_READ TRUE)

# Write compiler ID and version to the cache to allow retrieving this information
# with "cmake -LA" (see test-memory-leaks.py).
set(DOWNWARD_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID} CACHE STRING "")
set(DOWNWARD_CXX_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION} CACHE STRING "")
mark_as_advanced(DOWNWARD_CXX_COMPILER_ID DOWNWARD_CXX_COMPILER_VERSION)

# Add planner components as subprojects.

# Copy the translator into the output directory.
add_custom_target(translate ALL)
get_target_property(bin_dir fd_interface_library RUNTIME_OUTPUT_DIRECTORY)
add_custom_command(TARGET translate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/translate
        ${bin_dir}/${CMAKE_CFG_INTDIR}/translate
    COMMENT "Copying translator module into output directory")

add_subdirectory(search)
