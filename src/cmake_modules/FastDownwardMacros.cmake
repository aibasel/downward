include(CMakeParseArguments)

macro(fast_downward_set_compiler_flags)
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
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4456") # declaration hides previous local declaration
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458") # declaration hides class member

        # TODO: Configuration-specific flags
    endif()
endmacro()


macro(fast_downward_add_profile_build)
    if(NOT CMAKE_CONFIGURATION_TYPES)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;Profile")
    endif()
    set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_CXX_FLAGS_DEBUG})
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
endmacro()

macro(fast_downward_default_to_release_build)
    # Only for single-config generators (like Makefiles) that choose the build type at generation time.
    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        message("Defaulting to release build.")
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

macro(fast_downward_add_64_bit_option)
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
endmacro()

macro(fast_downward_check_64_bit_option)
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
endmacro()


function(fast_downward_plugin)
    set(_OPTIONS DEACTIVATED)
    set(_ONE_VALUE_ARGS NAME DISPLAY_NAME HELP)
    set(_MULTI_VALUE_ARGS SOURCES DEPENDS)
    cmake_parse_arguments(_PLUGIN "${_OPTIONS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )
    # Mandatory
    if(NOT _PLUGIN_NAME)
        message( FATAL_ERROR "fast_downward_plugin: 'NAME' argument required." )
    endif()
    if(NOT _PLUGIN_SOURCES)
        message( FATAL_ERROR "fast_downward_plugin: 'SOURCES' argument required." )
    endif()
    # Optional
    if(NOT _PLUGIN_DISPLAY_NAME)
        string(TOLOWER ${_PLUGIN_NAME} _PLUGIN_DISPLAY_NAME)
    endif()
    if(NOT _PLUGIN_HELP)
        set(_PLUGIN_HELP ${_PLUGIN_DISPLAY_NAME})
    endif()
    set(_OPTION_DEFAULT TRUE)
    if(_PLUGIN_DEACTIVATED)
        set(_OPTION_DEFAULT FALSE)
    endif()

    option(PLUGIN_${_PLUGIN_NAME}_ENABLED ${_PLUGIN_HELP} ${_OPTION_DEFAULT})

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
                set(PLUGIN_${DEPENDENCY}_ENABLED TRUE PARENT_SCOPE)
                list(APPEND _UNCHECKED_PLUGINS ${DEPENDENCY})
            endif()
        endforeach()
    endwhile()

    foreach(PLUGIN ${PLUGINS})
        if(PLUGIN_${PLUGIN}_ENABLED)
            message(STATUS "Using plugin: ${PLUGIN_${PLUGIN}_DISPLAY_NAME}")
            source_group(${PLUGIN_${PLUGIN}_DISPLAY_NAME} FILES "${PLUGIN_${PLUGIN}_SOURCES}")
            list(APPEND ${_SOURCES_LIST_VAR} "${PLUGIN_${PLUGIN}_SOURCES}")
        endif()
    endforeach()
    set(${_SOURCES_LIST_VAR} ${${_SOURCES_LIST_VAR}} PARENT_SCOPE)
endfunction()
