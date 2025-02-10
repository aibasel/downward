# Find the CPLEX LP solver and export the target cplex::cplex.
#
# Usage:
#  find_package(cplex)
#  target_link_libraries(<target> PRIVATE cplex::cplex)
#
# The location of CPLEX can be specified using the environment variable
# or cmake parameter cplex_DIR.
#
# The standard FIND_PACKAGE features are supported (QUIET, REQUIRED, etc.).

set(IMPORTED_CONFIGURATIONS "Debug" "Release")
set(HINT_PATHS ${cplex_DIR} $ENV{cplex_DIR})

if(WIN32)
    # On Windows we have to declare the library as SHARED to correctly
    # communicate the location of the dll and impllib files.
    add_library(cplex::cplex IMPORTED SHARED)
else()
    # On Linux, the CPLEX installer sometimes does not provide dynamic
    # libraries. If they are absent, we fall back to static ones further down,
    # hence we mark the type unknown here.
    add_library(cplex::cplex IMPORTED UNKNOWN)
endif()
set_target_properties(cplex::cplex PROPERTIES
    IMPORTED_CONFIGURATIONS "${IMPORTED_CONFIGURATIONS}"
)

find_path(CPLEX_INCLUDE_DIRS
    NAMES
    cplex.h
    HINTS
    ${HINT_PATHS}
    PATH_SUFFIXES
    include/ilcplex
)
target_include_directories(cplex::cplex INTERFACE ${CPLEX_INCLUDE_DIRS})

if(CPLEX_INCLUDE_DIRS)
    # Parse CPLEX version.
    file(STRINGS ${CPLEX_INCLUDE_DIRS}/cpxconst.h CPLEX_VERSION_STR
         REGEX "#define[ ]+CPX_VERSION[ ]+[0-9]+")
    string(REGEX MATCH "[0-9]+" CPLEX_VERSION_STR ${CPLEX_VERSION_STR})
    if(CPLEX_VERSION_STR)
      math(EXPR CPLEX_VERSION_MAJOR "${CPLEX_VERSION_STR} / 1000000")
      math(EXPR CPLEX_VERSION_MINOR "${CPLEX_VERSION_STR} / 10000 % 100")
      math(EXPR CPLEX_VERSION_SUBMINOR "${CPLEX_VERSION_STR} / 100 % 100")
      set(CPLEX_VERSION
          "${CPLEX_VERSION_MAJOR}.${CPLEX_VERSION_MINOR}.${CPLEX_VERSION_SUBMINOR}")
      set(CPLEX_VERSION_NO_DOTS
          "${CPLEX_VERSION_MAJOR}${CPLEX_VERSION_MINOR}${CPLEX_VERSION_SUBMINOR}")
    endif()

    # Versions >= 12.8 depend on dl.
    if(NOT (${CPLEX_VERSION} VERSION_LESS "12.8"))
        target_link_libraries(cplex::cplex INTERFACE ${CMAKE_DL_LIBS})
    endif()
else()
    set(CPLEX_VERSION "CPLEX_VERSION-NOTFOUND")
    set(CPLEX_VERSION_NO_DOTS "CPLEX_VERSION-NOTFOUND")
endif()

# Find dependencies.
set(FIND_OPTIONS)
if(${CPLEX_FIND_QUIETLY})
    list(APPEND FIND_OPTIONS "QUIET")
endif()
if(${CPLEX_FIND_REQUIRED})
    list(APPEND FIND_OPTIONS "REQUIRED")
endif()
find_package(Threads ${FIND_OPTIONS})
target_link_libraries(cplex::cplex INTERFACE Threads::Threads)


# CPLEX stores libraries under different paths of the form
# <PREFIX>/lib/<BITWIDTH_HINT>_<PLATFORM_HINT>[_<COMPILER_HINT>]/<LIBRARY_TYPE_HINT>/
# <PREFIX>/bin/<BITWIDTH_HINT>_<PLATFORM_HINT>[_<COMPILER_HINT>]/
# The hints have different options depending on the system and CPLEX version.
# We set up lists with all options and then multiply them out to set up
# possible paths where we should search for the library.

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(BITWIDTH_HINTS "x86")
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(BITWIDTH_HINTS "x86-64" "x64" "arm64")
endif()

if(APPLE)
    set(PLATFORM_HINTS "osx")
elseif(UNIX AND NOT APPLE) # starting from CMake >=3.25, use LINUX instead.
    set(PLATFORM_HINTS "linux" "sles10_4.1")
elseif(WIN32) # Despite the name, WIN32 is also true on 64-bit systems.
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(PLATFORM_HINTS "windows" "win32")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(PLATFORM_HINTS "windows" "win64")
    endif()
endif()

set(COMPILER_HINTS)
if(MSVC10)
    # Note that the numbers are correct: Visual Studio 2011 is version 10.
    set(COMPILER_HINTS "vs2011" "msvc10")
elseif(MSVC11)
    set(COMPILER_HINTS "vs2012" "msvc11")
elseif(MSVC12)
    set(COMPILER_HINTS "vs2013" "msvc12")
elseif(MSVC13)
    set(COMPILER_HINTS "vs2015" "msvc13")
elseif(MSVC14)
    set(COMPILER_HINTS "vs2017" "msvc14")
endif()

if(UNIX)
    set(LIBRARY_TYPE_HINTS_RELEASE "static_pic")
    set(LIBRARY_TYPE_HINTS_DEBUG "static_pic")
elseif(WIN32)
    set(LIBRARY_TYPE_HINTS_RELEASE "stat_mda")
    set(LIBRARY_TYPE_HINTS_DEBUG "stat_mdd")
endif()

set(REQUIRED_LIBRARIES)
foreach(CONFIG_ORIG ${IMPORTED_CONFIGURATIONS})
    # The configuration needs to be upper case in variable names like
    # IMPORTED_LOCATION_${CONFIG}.
    string(TOUPPER ${CONFIG_ORIG} CONFIG)

    # Collect possible suffixes.
    foreach(BITWIDTH_HINT ${BITWIDTH_HINTS})
        foreach(PLATFORM_HINT ${PLATFORM_HINTS})
            list(APPEND SUFFIXES_${CONFIG} "${BITWIDTH_HINT}_${PLATFORM_HINT}")
            foreach(LIBRARY_TYPE_HINT ${LIBRARY_TYPE_HINTS_${CONFIG}})
                list(APPEND SUFFIXES_${CONFIG} "${BITWIDTH_HINT}_${PLATFORM_HINT}/${LIBRARY_TYPE_HINT}")
                foreach(COMPILER_HINT ${COMPILER_HINTS})
                    list(APPEND SUFFIXES_${CONFIG} "${BITWIDTH_HINT}_${PLATFORM_HINT}_${COMPILER_HINT}/${LIBRARY_TYPE_HINT}")
                endforeach()
            endforeach()
        endforeach()
    endforeach()

    if (WIN32)
        # On Windows, libraries consist of a .dll file and a .lib file.
        # CPLEX stores the .dll file in /bin and the .lib file in /lib.
        # Since linking is against the .lib file, find_library() does not find
        # the dll and we have to use find_file() instead.
        find_file(CPLEX_SHARED_LIBRARY_${CONFIG}
            NAMES
            cplex${CPLEX_VERSION_NO_DOTS}.dll
            HINTS
            ${HINT_PATHS}/bin
            PATH_SUFFIXES
            ${SUFFIXES_${CONFIG}}
        )
        find_library(CPLEX_IMPLIB_${CONFIG}
            NAMES
            cplex${CPLEX_VERSION_NO_DOTS}
            HINTS
            ${HINT_PATHS}/lib
            PATH_SUFFIXES
            ${SUFFIXES_${CONFIG}}
        )
        set_target_properties(cplex::cplex PROPERTIES
            IMPORTED_LOCATION_${CONFIG} ${CPLEX_SHARED_LIBRARY_${CONFIG}}
            IMPORTED_IMPLIB_${CONFIG} ${CPLEX_IMPLIB_${CONFIG}}
        )
        list(APPEND REQUIRED_LIBRARIES CPLEX_SHARED_LIBRARY_${CONFIG} CPLEX_IMPLIB_${CONFIG})
    else()
        # CPLEX stores .so files in /bin
        find_library(CPLEX_LIBRARY_${CONFIG}
            NAMES
            cplex${CPLEX_VERSION_NO_DOTS}
            cplex
            HINTS
            ${HINT_PATHS}/bin
            ${HINT_PATHS}/lib
            PATH_SUFFIXES
            ${SUFFIXES_${CONFIG}}
        )
        set_target_properties(cplex::cplex PROPERTIES
            IMPORTED_LOCATION_${CONFIG} ${CPLEX_LIBRARY_${CONFIG}}
        )
        list(APPEND REQUIRED_LIBRARIES CPLEX_LIBRARY_${CONFIG})
    endif()
    endforeach()

# Check if everything was found and set CPLEX_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cplex
    REQUIRED_VARS CPLEX_INCLUDE_DIRS ${REQUIRED_LIBRARIES}
    THREADS_FOUND VERSION_VAR CPLEX_VERSION
)

mark_as_advanced(
    IMPORTED_CONFIGURATIONS HINT_PATHS CPLEX_INCLUDE_DIRS CPLEX_LIBRARIES
    CPX_VERSION CPLEX_VERSION_MAJOR CPLEX_VERSION_MINOR CPLEX_VERSION_STR
    CPLEX_VERSION_SUBMINOR CPLEX_VERSION_NO_DOTS BITWIDTH_HINTS PLATFORM_HINTS
    LIBRARY_TYPE_HINTS_RELEASE LIBRARY_TYPE_HINTS_DEBUG SUFFIXES_RELEASE
    SUFFIXES_DEBUG FIND_OPTIONS COMPILER_HINTS COMPILER_HINT CPLEX_IMPLIB_DEBUG
    CPLEX_IMPLIB_RELEASE CPLEX_LIBRARY_DEBUG CPLEX_LIBRARY_RELEASE
)
