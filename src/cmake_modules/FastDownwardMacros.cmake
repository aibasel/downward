include(CMakeParseArguments)


macro(define_interface_library)
    add_library(fd_interface_library INTERFACE)
    target_compile_features(fd_interface_library INTERFACE cxx_std_20)

    set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
    set(gcc_cxx "$<COMPILE_LANG_AND_ID:CXX,GNU>")
    set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
    set(gcc_like_release "$<AND:${gcc_like_cxx},$<CONFIG:RELEASE>>")
    set(gcc_like_debug "$<AND:${gcc_like_cxx},$<CONFIG:DEBUG>>")
    set(should_use_glibcxx_debug "$<AND:${gcc_like_debug},$<BOOL:USE_GLIBCXX_DEBUG>>")

    target_compile_options(fd_interface_library INTERFACE
        "$<${gcc_like_cxx}:-O3;-g>")
    target_compile_options(fd_interface_library INTERFACE
        "$<${gcc_like_release}:-DNDEBUG;-fomit-frame-pointer>")
    target_compile_definitions(fd_interface_library INTERFACE
        "$<${should_use_glibcxx_debug}:_GLIBCXX_DEBUG>")
    # Enable exceptions for MSVC.
    target_compile_options(fd_interface_library INTERFACE
        "$<${msvc_cxx}:/EHsc>")

    add_library(fd_warnings INTERFACE)
    target_compile_options(fd_warnings INTERFACE
        "$<${gcc_like_cxx}:-Wall;-Wextra;-Wpedantic;-Wnon-virtual-dtor;-Wfloat-conversion;-Wmissing-declarations;-Wzero-as-null-pointer-constant>")

    ## We ignore the warning "restrict" because of a bug in GCC 12:
    ## https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105651
    set(v12_or_later "$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,12>")
    set(before_v13 "$<VERSION_LESS:$<CXX_COMPILER_VERSION>,13>")
    set(bugged_gcc_cxx "$<AND:${gcc_cxx},${v12_or_later},${before_v13}>")
    target_compile_options(fd_warnings INTERFACE
        "$<${bugged_gcc_cxx}:-Wno-restrict>")

    # For MSVC, use warning level 4 (/W4) because /Wall currently detects too
    # many warnings outside of our code to be useful.
    target_compile_options(fd_warnings INTERFACE
        "$<${msvc_cxx}:/W4;/wd4456;/wd4458;/wd4459;/wd4244;/wd4267>")
        # Disable warnings that currently trigger in the code until we fix them.
        #   /wd4456: declaration hides previous local declaration
        #   /wd4458: declaration hides class member
        #   /wd4459: declaration hides global declaration
        #   /wd4244: conversion with possible loss of data
        #   /wd4267: conversion from size_t to int with possible loss of data
    target_link_libraries(fd_interface_library INTERFACE fd_warnings)

    # TODO: rework. The idea was that downward inherits this property, but it doesn't
    # Instead, maybe an "install" command would be sensible?
    set_target_properties(fd_interface_library PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endmacro()

# TODO: I'm not sure if we need this anymore? I could not find a corresponding
# "modern" command and it compiled without it
#~ macro(fast_downward_set_linker_flags)
    #~ if(UNIX)
        #~ set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
    #~ endif()
#~ endmacro()

# TODO: It seems this is not needed anymore? When I call build, it never enters
# the if block...
#~ macro(fast_downward_default_to_release_build)
    #~ # Only for single-config generators (like Makefiles) that choose the build type at generation time.
    #~ if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        #~ message(STATUS "Defaulting to release build.")
        #~ set(default_build_type "Release")
    #~ endif()
#~ endmacro()

# TODO: I cannot find out how to replace this set(CMAKE_* ...) call
macro(fast_downward_set_configuration_types)
    # Only for multi-config generators (like Visual Studio Projects) that choose
    # the build type at build time.
    if(CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release"
            CACHE STRING "Reset the configurations to what we need" FORCE)
    endif()
endmacro()

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

function(fast_downward_plugin)
    set(_OPTIONS DEPENDENCY_ONLY CORE_PLUGIN)
    set(_ONE_VALUE_ARGS NAME DISPLAY_NAME HELP)
    set(_MULTI_VALUE_ARGS SOURCES DEPENDS)
    cmake_parse_arguments(_PLUGIN "${_OPTIONS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})
    # Check mandatory arguments.
    if(NOT _PLUGIN_NAME)
        message(FATAL_ERROR "fast_downward_plugin: 'NAME' argument required.")
    endif()
    if(NOT _PLUGIN_SOURCES)
        message(FATAL_ERROR "fast_downward_plugin: 'SOURCES' argument required.")
    endif()

    fast_downward_add_existing_sources_to_list(_PLUGIN_SOURCES)

    if (NOT _PLUGIN_CORE_PLUGIN AND NOT _PLUGIN_DEPENDENCY_ONLY)
        # Decide whether the plugin should be enabled by default.
        if (DISABLE_PLUGINS_BY_DEFAULT)
            set(_OPTION_DEFAULT FALSE)
        else()
            set(_OPTION_DEFAULT TRUE)
        endif()
        option(PLUGIN_${_PLUGIN_NAME}_ENABLED ${_PLUGIN_HELP} ${_OPTION_DEFAULT})
    endif()

    add_library(downward_${_PLUGIN_NAME} INTERFACE)
    target_link_libraries(downward_${_PLUGIN_NAME} INTERFACE fd_interface_library)
    target_sources(downward_${_PLUGIN_NAME} INTERFACE ${_PLUGIN_SOURCES})
    foreach(DEPENDENCY ${_PLUGIN_DEPENDS})
        target_link_libraries(downward_${_PLUGIN_NAME} INTERFACE downward_${DEPENDENCY})
    endforeach()

    if (_PLUGIN_CORE_PLUGIN OR PLUGIN_${_PLUGIN_NAME}_ENABLED)
        target_link_libraries(downward PUBLIC downward_${_PLUGIN_NAME})
    endif()
endfunction()
