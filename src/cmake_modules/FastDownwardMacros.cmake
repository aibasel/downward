include(CMakeParseArguments)

macro(fast_downward_set_compiler_flags)
    # Note: on CMake >= 3.0 the compiler ID of Apple-provided clang is AppleClang.
    # If we change the required CMake version from 2.8.3 to 3.0 or greater,
    # we have to fix this.
    if(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag( "-std=c++11" CXX11_FOUND )
        if(CXX11_FOUND)
             set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        else()
            message(FATAL_ERROR "${CMAKE_CXX_COMPILER} does not support C++11, please use a different compiler")
        endif()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wnon-virtual-dtor")

        ## Configuration-specific flags
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
        set(CMAKE_CXX_FLAGS_DEBUG "-O3 -D_GLIBCXX_DEBUG")
        set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg")
    elseif(MSVC)
        # We force linking to be static on Windows because this makes compiling OSI simpler
        # (dynamic linking would require DLLs for OSI). On Windows this is a compiler
        # setting, not a linker setting.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        # Enable exceptions.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

        # Use warning level 4 (/W4).
        # /Wall currently detects too many warnings outside of our code to be useful.
        string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        # Disable warnings that currently trigger in the code until we fix them.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800") # forcing value to bool
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4512") # assignment operator could not be generated
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4706") # assignment within conditional expression (in tree.hh)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unreferenced formal parameter (in OSI)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127") # conditional expression is constant (in tree.hh and in our code)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244") # conversion with possible loss of data
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4309") # truncation of constant value (in OSI, see issue857)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4702") # unreachable code
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4239") # nonstandard extension used
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996") # function call with parameters that may be unsafe
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4456") # declaration hides previous local declaration
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458") # declaration hides class member
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267") # conversion from size_t to int with possible loss of data

        # The following are disabled because of what seems to be compiler bugs.
        # "unreferenced local function has been removed";
        # see http://stackoverflow.com/questions/3051992/compiler-warning-at-c-template-base-class
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4505")

        # TODO: Configuration-specific flags. We currently rely on the fact that
        # CMAKE_CXX_FLAGS_RELEASE and CMAKE_CXX_FLAGS_DEBUG get reasonable settings
        # from cmake. This is the case for most build environments, but we have less
        # control over the way the binary is created.
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER}")
    endif()
endmacro()

macro(fast_downward_set_linker_flags)
    if(UNIX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
    endif()
endmacro()

macro(fast_downward_add_profile_build)
    # We don't offer a dedicated PROFILE build on Windows.
    if(CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        if(NOT CMAKE_CONFIGURATION_TYPES)
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;Profile")
        endif()
        set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_CXX_FLAGS_DEBUG})
        set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pg")
    endif()
endmacro()

macro(fast_downward_default_to_release_build)
    # Only for single-config generators (like Makefiles) that choose the build type at generation time.
    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        message(STATUS "Defaulting to release build.")
        set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    endif()
endmacro()

macro(fast_downward_set_configuration_types)
    # Only for multi-config generators (like Visual Studio Projects) that choose
    # the build type at build time.
    if(CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile"
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
