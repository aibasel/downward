include(CMakeParseArguments)



add_library(cxx_options INTERFACE)
target_compile_features(cxx_options INTERFACE cxx_std_20)

set(using_gcc_like "$<CXX_COMPILER_ID:ARMClang,AppleClang,Clang,GNU,LCC>")
set(using_gcc "$<CXX_COMPILER_ID:GNU>")
set(using_msvc "$<CXX_COMPILER_ID:MSVC>")
set(using_gcc_like_release "$<AND:${using_gcc_like},$<CONFIG:RELEASE>>")
set(using_gcc_like_debug "$<AND:${using_gcc_like},$<CONFIG:DEBUG>>")
set(should_use_glibcxx_debug "$<AND:${using_gcc_like_debug},$<BOOL:USE_GLIBCXX_DEBUG>>")

target_compile_options(cxx_options INTERFACE
    "$<${using_gcc_like}:-O3;-g>")
target_link_options(cxx_options INTERFACE
    "$<${using_gcc_like}:-g>")
target_compile_options(cxx_options INTERFACE
    "$<${using_gcc_like_release}:-DNDEBUG;-fomit-frame-pointer>")
target_compile_definitions(cxx_options INTERFACE
    "$<${should_use_glibcxx_debug}:_GLIBCXX_DEBUG>")
# Enable exceptions for MSVC.
target_compile_options(cxx_options INTERFACE
    "$<${using_msvc}:/EHsc>")

add_library(cxx_warnings INTERFACE)
target_compile_options(cxx_warnings INTERFACE
    "$<${using_gcc_like}:-Wall;-Wextra;-Wpedantic;-Wnon-virtual-dtor;-Wfloat-conversion;-Wmissing-declarations;-Wzero-as-null-pointer-constant>")

## We ignore the warning "restrict" because of a bug in GCC 12:
## https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105651
set(v12_or_later "$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,12>")
set(before_v13 "$<VERSION_LESS:$<CXX_COMPILER_VERSION>,13>")
set(bugged_gcc "$<AND:${using_gcc},${v12_or_later},${before_v13}>")
target_compile_options(cxx_warnings INTERFACE
    "$<${bugged_gcc}:-Wno-restrict>")

# For MSVC, use warning level 4 (/W4) because /Wall currently detects too
# many warnings outside of our code to be useful.
target_compile_options(cxx_warnings INTERFACE
    "$<${using_msvc}:/W4;/wd4456;/wd4458;/wd4459;/wd4244;/wd4267>")
    # Disable warnings that currently trigger in the code until we fix them.
    #   /wd4456: declaration hides previous local declaration
    #   /wd4458: declaration hides class member
    #   /wd4459: declaration hides global declaration
    #   /wd4244: conversion with possible loss of data
    #   /wd4267: conversion from size_t to int with possible loss of data
target_link_libraries(cxx_options INTERFACE cxx_warnings)


function(fast_downward_set_up_build_types)
    set(allowedBuildTypes Debug Release)

    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(isMultiConfig)
        # Set the possible choices for multi-config generators like (like
        # Visual Studio Projects) that choose the build type at build time.
        set(CMAKE_CONFIGURATION_TYPES "${allowedBuildTypes}"
            CACHE STRING "Supported build types: ${allowedBuildTypes}" FORCE)
    else()
        # Set the possible choices for programs like ccmake.
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")
        if(NOT CMAKE_BUILD_TYPE)
            message(STATUS "Defaulting to release build.")
            set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
        elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
            message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}. "
                "Supported build types: ${allowedBuildTypes}")
        endif()
    endif()
endfunction()


macro(fast_downward_report_bitwidth)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        message(STATUS "Building for 32-bit.")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        message(STATUS "Building for 64-bit.")
    else()
        message(FATAL_ERROR, "Could not determine bitwidth.")
    endif()
endmacro()

function(fast_downward_add_existing_sources_to_list _SOURCES_LIST_VAR)
    set(_ALL_FILES)
    foreach(SOURCE_FILE ${${_SOURCES_LIST_VAR}})
        get_filename_component(_SOURCE_FILE_DIR ${SOURCE_FILE} PATH)
        get_filename_component(_SOURCE_FILE_NAME ${SOURCE_FILE} NAME_WE)
        get_filename_component(_SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
        if (_SOURCE_FILE_DIR)
            set(_SOURCE_FILE_DIR "${_SOURCE_FILE_DIR}/")
        endif()
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_SOURCE_FILE_DIR}${_SOURCE_FILE_NAME}.h")
            list(APPEND _ALL_FILES "${_SOURCE_FILE_DIR}${_SOURCE_FILE_NAME}.h")
        endif()
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_SOURCE_FILE_DIR}${_SOURCE_FILE_NAME}.cc")
            list(APPEND _ALL_FILES "${_SOURCE_FILE_DIR}${_SOURCE_FILE_NAME}.cc")
        endif()
    endforeach()
    set(${_SOURCES_LIST_VAR} ${_ALL_FILES} PARENT_SCOPE)
endfunction()

function(create_fast_downward_library)
    set(_OPTIONS DEPENDENCY_ONLY CORE_LIBRARY)
    set(_ONE_VALUE_ARGS NAME DISPLAY_NAME HELP)
    set(_MULTI_VALUE_ARGS SOURCES DEPENDS)
    cmake_parse_arguments(_LIBRARY "${_OPTIONS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})
    # Check mandatory arguments.
    if(NOT _LIBRARY_NAME)
        message(FATAL_ERROR "fast_downward_library: 'NAME' argument required.")
    endif()
    if(NOT _LIBRARY_SOURCES)
        message(FATAL_ERROR "fast_downward_library: 'SOURCES' argument required.")
    endif()

    fast_downward_add_existing_sources_to_list(_LIBRARY_SOURCES)

    if (NOT _LIBRARY_CORE_LIBRARY AND NOT _LIBRARY_DEPENDENCY_ONLY)
        # Decide whether the plugin should be enabled by default.
        if (DISABLE_LIBRARIES_BY_DEFAULT)
            set(_OPTION_DEFAULT FALSE)
        else()
            set(_OPTION_DEFAULT TRUE)
        endif()
        option(LIBRARY_${_LIBRARY_NAME}_ENABLED ${_LIBRARY_HELP} ${_OPTION_DEFAULT})
    endif()

    add_library(downward_${_LIBRARY_NAME} INTERFACE)
    target_link_libraries(downward_${_LIBRARY_NAME} INTERFACE cxx_options)
    target_sources(downward_${_LIBRARY_NAME} INTERFACE ${_LIBRARY_SOURCES})
    foreach(DEPENDENCY ${_LIBRARY_DEPENDS})
        target_link_libraries(downward_${_LIBRARY_NAME} INTERFACE downward_${DEPENDENCY})
    endforeach()

    if (_LIBRARY_CORE_LIBRARY OR LIBRARY_${_LIBRARY_NAME}_ENABLED)
        target_link_libraries(downward PUBLIC downward_${_LIBRARY_NAME})
    endif()
endfunction()
