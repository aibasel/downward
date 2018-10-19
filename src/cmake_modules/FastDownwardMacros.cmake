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
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wnon-virtual-dtor -Werror")

        ## Configuration-specific flags
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
        set(CMAKE_CXX_FLAGS_DEBUG "-O3 -D_GLIBCXX_DEBUG")
        set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg")
    elseif(MSVC)
        # We force linking to be static because the dynamically linked code is
        # about 10% slower on Linux (see issue67). On Windows this is a compiler
        # setting, not a linker setting.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        # Enable exceptions.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

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

    # Fixing the linking to static or dynamic is only supported on Unix.
    # We don't support the option on MacOS because static linking is
    # not supported by Apple: https://developer.apple.com/library/mac/qa/qa1118/_index.html
    # We don't support it on Windows because we don't have a use case
    # and it's not possible to distinguish static and dynamic libraries
    # by their file name.
    if(FORCE_DYNAMIC_BUILD AND NOT UNIX)
        message(FATAL_ERROR "Forcing dynamic builds is only supported on Unix.")
    endif()

    if(UNIX AND NOT APPLE)
        # By default, we try to force linking to be static because the
        # dynamically linked code is about 10% slower on Linux (see issue67)
        # but we offer the option to force a dynamic build for debugging
        # purposes (for example, valgrind's memcheck requires a dynamic build).
        # To force a dynamic build, set FORCE_DYNAMIC_BUILD to true by passing
        # -DFORCE_DYNAMIC_BUILD=YES to cmake. We do not introduce an option for
        # this because it cannot be changed after the first cmake run.
        if(FORCE_DYNAMIC_BUILD)
            message(STATUS "Forcing dynamic build.")

            # Any libraries that are implicitly added to the end of the linker
            # command should be linked dynamically.
            set(LINK_SEARCH_END_STATIC FALSE)

            # Only look for dynamic libraries.
            set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
        else()
            message(STATUS "Forcing static build.")

            # Any libraries that are implicitly added to the end of the linker
            # command should be linked statically.
            set(LINK_SEARCH_END_STATIC TRUE)

            # Only look for static libraries.
            set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

            # Set linker flags to link statically.
            if(CMAKE_COMPILER_IS_GNUCXX)
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc")
            elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++")

                # CMake automatically adds the -rdynamic flag to the
                # following two variables, which causes an error in our
                # static builds with clang. Therefore we explicitly
                # clear the variables.
                set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
                set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
            endif()
        endif()
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

macro(fast_downward_set_bitwidth)
    # Add -m32 to the compiler flags on Unix, unless ALLOW_64_BIT is set to true.
    # This has to be done before defining the project.

    # Since compiling for 32-bit works differently on each platform, we let
    # users set up their own build environment and only check which one is
    # used. Compiling a 64-bit version of the planner without explicitly
    # settig ALLOW_64_BIT to true results in an error.
    option(ALLOW_64_BIT "Allow to compile a 64-bit version." FALSE)

    if(UNIX AND NOT ALLOW_64_BIT)
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "c++ flags")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32" CACHE STRING "c flags")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32" CACHE STRING "linker flags")
    endif()
endmacro()

macro(fast_downward_check_64_bit_option)
    # The macro fast_downward_set_bitwidth
    # adds -m32 to the compiler flags on Unix, unless ALLOW_64_BIT is
    # set to true. If this done before defining a project, the tool
    # chain will be set up for 32-bit and CMAKE_SIZEOF_VOID_P should be 4.
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        if(ALLOW_64_BIT)
            message(WARNING "Building for 32-bit but ALLOW_64_BIT is set. "
            "Do not set ALLOW_64_BIT unless you are sure you want a 64-bit build. "
            "See http://www.fast-downward.org/PlannerUsage#A64bit for details.")
        else()
            message(STATUS "Building for 32-bit.")
        endif()
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        if(ALLOW_64_BIT)
            message(STATUS "Building for 64-bit.")
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
