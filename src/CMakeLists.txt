###   * Usage:
###         mkdir -p builds/release32
###         cd builds/release32
###         cmake path/to/src
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

# Path containing custom CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
include(FastDownwardMacros)

fast_downward_set_configuration_types()
fast_downward_add_64_bit_option()
project(fast-downward)
fast_downward_check_64_bit_option()

set(FAST_DOWNWARD_MAIN_CMAKELISTS_READ TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_custom_target(translator ALL)
add_custom_command(TARGET translator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/translate
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translate
    COMMENT "Copying translator module to output directory")

add_subdirectory(VAL)
add_subdirectory(preprocess)
add_subdirectory(search)
