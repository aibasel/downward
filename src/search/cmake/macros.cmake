include_guard(GLOBAL)

include(CMakeParseArguments)

function(set_up_build_types allowedBuildTypes)
    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(isMultiConfig)
        # Set the possible choices for multi-config generators (like Visual
        # Studio Projects) that choose the build type at build time.
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

macro(report_bitwidth)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        message(STATUS "Building for 32-bit.")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        message(STATUS "Building for 64-bit.")
    else()
        message(FATAL_ERROR, "Could not determine bitwidth.")
    endif()
endmacro()

function(add_existing_sources_to_list _SOURCES_LIST_VAR)
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
    set(_ONE_VALUE_ARGS NAME HELP)
    set(_MULTI_VALUE_ARGS SOURCES DEPENDS)
    cmake_parse_arguments(_LIBRARY "${_OPTIONS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})
    # Check mandatory arguments.
    if(NOT _LIBRARY_NAME)
        message(FATAL_ERROR "fast_downward_library: 'NAME' argument required.")
    endif()
    if(NOT _LIBRARY_SOURCES)
        message(FATAL_ERROR "fast_downward_library: 'SOURCES' argument required.")
    endif()

    add_existing_sources_to_list(_LIBRARY_SOURCES)

    if (NOT _LIBRARY_CORE_LIBRARY AND NOT _LIBRARY_DEPENDENCY_ONLY)
        # Decide whether the library should be enabled by default.
        if (DISABLE_LIBRARIES_BY_DEFAULT)
            set(_OPTION_DEFAULT FALSE)
        else()
            set(_OPTION_DEFAULT TRUE)
        endif()
        string(TOUPPER ${_LIBRARY_NAME} _LIBRARY_NAME_UPPER)
        option(LIBRARY_${_LIBRARY_NAME_UPPER}_ENABLED ${_LIBRARY_HELP} ${_OPTION_DEFAULT})
    endif()

    add_library(${_LIBRARY_NAME} INTERFACE)
    target_link_libraries(${_LIBRARY_NAME} INTERFACE common_cxx_flags)
    target_sources(${_LIBRARY_NAME} INTERFACE ${_LIBRARY_SOURCES})
    target_link_libraries(${_LIBRARY_NAME} INTERFACE ${_LIBRARY_DEPENDS})

    if (_LIBRARY_CORE_LIBRARY OR LIBRARY_${_LIBRARY_NAME_UPPER}_ENABLED)
        target_link_libraries(downward PUBLIC ${_LIBRARY_NAME})
    endif()
endfunction()

function(copy_dlls_to_binary_dir_after_build _TARGET_NAME)
    # Once we require CMake version >=3.21, we can use the code below instead.
    # https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#genex:TARGET_RUNTIME_DLLS
    # add_custom_command(TARGET ${_TARGET_NAME} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${_TARGET_NAME}> $<TARGET_RUNTIME_DLLS:${_TARGET_NAME}>
    #     COMMAND_EXPAND_LISTS
    # )
    # On top of making the variables and dummy hack below obsolete, this
    # solution will also automatically detect all DLLs our executable depends
    # on, so we won't have to handle CPLEX explicitly anymore.
    set(_has_cplex_target "$<TARGET_EXISTS:cplex::cplex>")

    set(_is_release_build "$<CONFIG:Release>")
    set(_release_dll "$<TARGET_PROPERTY:cplex::cplex,IMPORTED_LOCATION_RELEASE>")
    set(_has_release_dll "$<BOOL:${_release_dll}>")
    set(_should_copy_release_dll "$<AND:${_has_cplex_target},${_is_release_build},${_has_release_dll}>")

    set(_is_debug_build "$<CONFIG:Debug>")
    set(_debug_dll "$<TARGET_PROPERTY:cplex::cplex,IMPORTED_LOCATION_DEBUG>")
    set(_has_debug_dll "$<BOOL:${_debug_dll}>")
    set(_should_copy_debug_dll "$<AND:${_has_cplex_target},${_is_debug_build},${_has_debug_dll}>")

    # In case no DLL file has to be copied, the copy command below would get an
    # empty list of files to copy, and it would crash. We cannot use an "if"
    # because the result of the generator expressions will only be known at
    # build time. We thus always also copy the target to its own location.
    set(_dummy "$<TARGET_FILE:${_TARGET_NAME}>")

    add_custom_command(TARGET ${_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<${_should_copy_release_dll}:${_release_dll}>
            $<${_should_copy_debug_dll}:${_debug_dll}>
            ${_dummy}
            $<TARGET_FILE_DIR:${_TARGET_NAME}>
            COMMAND_EXPAND_LISTS
    )
endfunction()
