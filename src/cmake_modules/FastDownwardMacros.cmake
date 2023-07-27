include(CMakeParseArguments)


macro(define_interface_library)
    add_library(fd_interface_library INTERFACE)
    target_compile_features(fd_interface_library INTERFACE cxx_std_20)
    # TODO: rework. The idea was that downward inherits this property, but it doesn't
    # Instead, maybe an "install" command would be sensible?
    set_target_properties(fd_interface_library PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

    # TODO: check if these are the correct names
    set(gcc_cxx "$<COMPILE_LANG_AND_ID:CXX,GNU,LCC>")
    set(clang_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang>")
    set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

    if(gcc_cxx OR clang_cxx)
        target_compile_options(fd_interface_library INTERFACE "-g")
        target_compile_options(fd_interface_library INTERFACE "-Wall;-Wextra;-Wpedantic;-Wnon-virtual-dtor;-Wfloat-conversion;-Wmissing-declarations;-Wzero-as-null-pointer-constant")

        if (gcc_cxx
            AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12
            AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
            ## We ignore the warning "restrict" because of a bug in GCC 12:
            ## https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105651
            target_compile_options(fd_interface_library INTERFACE "-Wno-restrict")
        endif()

        ## Configuration-specific flags
        target_compile_options(fd_interface_library INTERFACE "$<$<CONFIG:RELEASE>:-O3;-DNDEBUG;-fomit-frame-pointer>")
        target_compile_options(fd_interface_library INTERFACE "$<$<CONFIG:DEBUG>:-O3>")
        if(USE_GLIBCXX_DEBUG)
            target_compile_options(fd_interface_library INTERFACE "$<$<CONFIG:DEBUG>:-D_GLIBCXX_DEBUG>")
        endif()
    elseif(msvc_cxx)
        # Enable exceptions.
        target_compile_options(fd_interface_library INTERFACE "/EHsc")        

        # Use warning level 4 (/W4).
        # /Wall currently detects too many warnings outside of our code to be useful.
        target_compile_options(fd_interface_library INTERFACE "/W4")
        # TODO: before, we replaced W3 with W4. Do we still need to do that?

        # Disable warnings that currently trigger in the code until we fix them.
        target_compile_options(fd_interface_library INTERFACE "/wd4456") # declaration hides previous local declaration
        target_compile_options(fd_interface_library INTERFACE "/wd4458") # declaration hides class member
        target_compile_options(fd_interface_library INTERFACE "/wd4459") # declaration hides global declaration
        target_compile_options(fd_interface_library INTERFACE "/wd4244") # conversion with possible loss of data
        target_compile_options(fd_interface_library INTERFACE "/wd4267") # conversion from size_t to int with possible loss of data

        # TODO: Configuration-specific flags. We currently rely on the fact that
        # CMAKE_CXX_FLAGS_RELEASE and CMAKE_CXX_FLAGS_DEBUG get reasonable settings
        # from cmake. This is the case for most build environments, but we have less
        # control over the way the binary is created.
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER}")
    endif()
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
    # Check optional arguments.
    if(NOT _PLUGIN_DISPLAY_NAME)
        string(TOLOWER ${_PLUGIN_NAME} _PLUGIN_DISPLAY_NAME)
    endif()
    if(NOT _PLUGIN_HELP)
        set(_PLUGIN_HELP ${_PLUGIN_DISPLAY_NAME})
    endif()
    # Decide whether the plugin should be enabled by default.
    if (DISABLE_PLUGINS_BY_DEFAULT)
        set(_OPTION_DEFAULT FALSE)
    else()
        set(_OPTION_DEFAULT TRUE)
    endif()
    # Overwrite default value for core plugins and dependecy-only plugins.
    if (_PLUGIN_CORE_PLUGIN)
        set(_OPTION_DEFAULT TRUE)
    elseif(_PLUGIN_DEPENDENCY_ONLY)
        set(_OPTION_DEFAULT FALSE)
    endif()

    option(PLUGIN_${_PLUGIN_NAME}_ENABLED ${_PLUGIN_HELP} ${_OPTION_DEFAULT})
    if(_PLUGIN_DEPENDENCY_ONLY OR _PLUGIN_CORE_PLUGIN)
        mark_as_advanced(PLUGIN_${_PLUGIN_NAME}_ENABLED)
    endif()

    set(PLUGIN_${_PLUGIN_NAME}_DISPLAY_NAME ${_PLUGIN_DISPLAY_NAME} PARENT_SCOPE)
    set(PLUGIN_${_PLUGIN_NAME}_SOURCES ${_PLUGIN_SOURCES} PARENT_SCOPE)
    set(PLUGIN_${_PLUGIN_NAME}_DEPENDS ${_PLUGIN_DEPENDS} PARENT_SCOPE)
    list(APPEND PLUGINS ${_PLUGIN_NAME})
    set(PLUGINS ${PLUGINS} PARENT_SCOPE)
endfunction()

function(fast_downward_add_plugin_sources _SOURCES_LIST_VAR)
    set(_UNCHECKED_PLUGINS)
    foreach(PLUGIN ${PLUGINS})
        if(PLUGIN_${PLUGIN}_ENABLED)
            list(APPEND _UNCHECKED_PLUGINS ${PLUGIN})
        endif()
    endforeach()

    while(_UNCHECKED_PLUGINS)
        list(GET _UNCHECKED_PLUGINS 0 PLUGIN)
        list(REMOVE_AT _UNCHECKED_PLUGINS 0)
        foreach(DEPENDENCY ${PLUGIN_${PLUGIN}_DEPENDS})
            if (NOT PLUGIN_${DEPENDENCY}_ENABLED)
                message(STATUS "Enabling plugin ${PLUGIN_${DEPENDENCY}_DISPLAY_NAME} "
                        "because plugin ${PLUGIN_${PLUGIN}_DISPLAY_NAME} is enabled and depends on it.")
                set(PLUGIN_${DEPENDENCY}_ENABLED TRUE)
                set(PLUGIN_${DEPENDENCY}_ENABLED TRUE PARENT_SCOPE)
                list(APPEND _UNCHECKED_PLUGINS ${DEPENDENCY})
            endif()
        endforeach()
    endwhile()

    foreach(PLUGIN ${PLUGINS})
        if(PLUGIN_${PLUGIN}_ENABLED)
            message(STATUS "Using plugin: ${PLUGIN_${PLUGIN}_DISPLAY_NAME}")
            source_group(${PLUGIN_${PLUGIN}_DISPLAY_NAME} FILES ${PLUGIN_${PLUGIN}_SOURCES})
            list(APPEND ${_SOURCES_LIST_VAR} "${PLUGIN_${PLUGIN}_SOURCES}")
        endif()
    endforeach()
    set(${_SOURCES_LIST_VAR} ${${_SOURCES_LIST_VAR}} PARENT_SCOPE)
endfunction()
