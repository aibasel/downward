include_guard(GLOBAL)

function(set_up_options)
    option(
        USE_GLIBCXX_DEBUG
        "Enable the libstdc++ debug mode that does additional safety checks. (On Linux \
systems, g++ and clang++ usually use libstdc++ for the C++ library.) The checks \
come at a significant performance cost and should only be enabled in debug mode. \
Enabling them makes the binary incompatible with libraries that are not compiled \
with this flag, which can lead to hard-to-debug errors."
        FALSE)

    option(
        USE_LP
        "Compile with support for all LP solvers installed on this system. \
If any enabled library requires an LP solver, compile with all \
available LP solvers. If no solvers are installed, the planner will \
still compile, but using heuristics that depend on an LP solver will \
cause an error. This behavior can be overwritten by setting the \
option USE_LP to false."
        TRUE)

    if(USE_GLIBCXX_DEBUG AND USE_LP)
        message(
            FATAL_ERROR
            "To prevent incompatibilities, the option USE_GLIBCXX_DEBUG is "
            "not supported when an LP solver is used. See issue982 for details.")
    endif()

    option(
        DISABLE_LIBRARIES_BY_DEFAULT
        "If set to YES only libraries that are specifically enabled will be compiled"
        NO)
    # This option should not show up in CMake GUIs like ccmake where all
    # libraries are enabled or disabled manually.
    mark_as_advanced(DISABLE_LIBRARIES_BY_DEFAULT)
endfunction()
