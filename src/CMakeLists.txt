# Usage:
#         mkdir -p builds/release32
#         cd builds/release32
#         cmake path/to/src
#         make [-j4]
# The call to cmake caches settings in the build directory and reads
# them from the cache on subsequent builds. If you want to change the
# settings of some options, do _not_ change the CMakeLIsts.txt files.
# Instead, create a new build directory, pass -DMY_OPTION=my_value to
# cmake. Alternatively, you can use a cmake GUI like ccmake to edit
# the cache.
#
# Three build targets are defined:
#
# * release (default)
#      -O3 optimisation, debugging symbols, assertions inactive
# * debug
#      -O3 optimisation, full debugging information, assertions active
# * profile
#      like Debug but with profile information linked in
#
# In all build targets, we overwrite the default configuration to
# include "-g", allow cross compilation and switch to pedantic error
# reporting.
#
# You can change the build target, by adding the parameter
#   -DCMAKE_BUILD_TYPE=type
# to the cmake call.

# Version 2.8.3 introduces CMakeParseArguments.
cmake_minimum_required(VERSION 2.8.3)

# Respect the PATH environment variable when searching for compilers.
find_program(CMAKE_C_COMPILER NAMES $ENV{CC} gcc PATHS ENV PATH NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} g++ PATHS ENV PATH NO_DEFAULT_PATH)

# Path containing custom CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
include(FastDownwardMacros)

fast_downward_default_to_release_build()
# We have to set the desired bitwidth before we can define the project.
fast_downward_set_bitwidth()
project(fast-downward)
fast_downward_check_64_bit_option()
# Due to a bug in cmake, configuration types are only set up correctly on the second cmake run.
# This means that cmake has to be called twice for multi-config generators like Visual Studio.
fast_downward_set_configuration_types()
fast_downward_add_profile_build()

set(FAST_DOWNWARD_MAIN_CMAKELISTS_READ TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)


# Add planner components as subprojects.

# Copy the translator into the output directory.
add_custom_target(translate ALL)
add_custom_command(TARGET translate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/translate
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/translate
    COMMENT "Copying translator module into output directory")

add_subdirectory(search)
