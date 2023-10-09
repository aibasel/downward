= Obtaining and running Fast Downward =

This page describes how to build and run the Fast Downward planner.


== Supported platforms ==

The planner is mainly developed under Linux; and all of its features should work with no restrictions under this platform.
The planner should compile and run correctly on macOS, but we cannot guarantee that it works as well as under Linux.
The same comment applies for Windows, where additionally some diagnostic features (e.g. reporting memory used when the planner is terminated by a signal) are not supported.
Setting time and memory limits and running portfolios is not supported under Windows either.

We appreciate bug reports and patches for all platforms, in particular, contributions to the documentation (e.g. installation instructions) for macOS and Windows.


== Dependencies ==

'''Linux:''' To obtain and build the planner, you will need the Git version control system, a C++ compiler, CMake and GNU make.
To run the planner, you will also need Python 3.
On Linux, the following should install all these dependencies: {{{
sudo apt install cmake g++ git make python3}}}

'''Windows:''' If using Windows, you should install [[https://visualstudio.microsoft.com/de/vs/older-downloads/|Visual Studio >= 2017]],
[[https://www.python.org/downloads/windows/|Python]], [[https://git-scm.com/download/win|Git]], and [[http://www.cmake.org/download/|CMake]].
During the installation of Visual Studio, the C++ compiler is not installed by default, but the IDE will prompt you to install it when you create a new C++ project.


=== Linear-Programming configurations ===

Some configurations require an LP solver to work. The planner will compile fine if there is no LP installed on the system, but trying to use the features that require an LP solver will generate an error message explaining what is missing.
See [[LPBuildInstructions]] for instructions on how to set up an LP solver and tell Fast Downward about it.

=== Validating the computed plans ===

You can validate the found plans by passing {{{--validate}}} to the planner. For this, the [[https://github.com/KCL-Planning/VAL|VAL plan validation software]]
needs to be installed on your system. [[SettingUpVal|Here]] you can find some instructions to help you set it up.


== Obtaining the code ==

Since July 2020, the main development of the code takes place on [[https://github.com/aibasel/downward/|Github]] but [[Releases#Historical_Releases|legacy versions of previous repositories]] are available.
The command {{{
git clone https://github.com/aibasel/downward.git DIRNAME}}}
will create a clone of the Fast Downward master repository in directory {{{DIRNAME}}}. The directory is created if it does not yet exist. In the following, we assume that you used {{{downward}}} as the {{{DIRNAME}}}.

== Compiling the planner ==

To build the planner for the first time, run:
{{{#!highlight bash
cd downward
./build.py}}}

This will create our default build {{{release}}} in the directory {{{downward/builds}}}. Other predefined build types are {{{debug}}}, {{{release_no_lp}}}, {{{glibcxx_debug}}} and {{{minimal}}}. Calling {{{./build.py --debug}}} will create a default debug build (equivalent to {{{debug}}}). You can pass make parameters to {{{./build.py}}}, e.g., {{{./build.py --debug -j4}}} will create a debug build using 4 threads for compilation ({{{-j4}}}), and {{{./build.py translate}}} will only build the translator component. (Because the translator is implemented in Python, "building" it just entails copying its source code into the build directory.) By default, {{{build.py}}} uses all cores for building the planner.

See [[#Manual_Builds]] for more complex builds.

=== Compiling on macOS ===

Fast Downward should compile with the GNU C++ compiler and {{{clang}}} with the same instructions described above. In case your system compiler does not work for some reason, you will need to install the GNU compiler (if not already present) and tell CMake which compiler to use (see [[#Manual_Builds]]).

=== Compiling on Windows ===

Windows does not interpret the shebang in Python files, so you have to call {{{build.py}}} as {{{python3 build.py}}} (make sure python3 is on your {{{PATH}}}). Also note that options are passed without {{{--}}}, e.g., {{{python3 build.py build=release_no_lp}}}.

Note that compiling from terminal is only possible with the right environment. The easiest way to get such an environment is to use the ``Developer Power''''''Shell for VS 2019`` or ``Developer Power''''''Shell.

Alternatively, you can create a Visual Studio Project (see [[#Manual_Builds]]), open it in Visual Studio and build from there. Visual Studio will create its binary files in subdirectories of the project that our driver script currently does not recognize. If you build with Visual Studio, you will have to run the individual components of the planner yourself.

== Manual and Custom Builds ==

The {{{build.py}}} script only creates a directory, calls {{{cmake}}} once to generate a build system, and a second time to execute the build. To do these steps manually, run:
{{{#!highlight bash
cmake -S src -B builds/mycustombuild
cmake --build builds/mycustombuild}}}

where {{{CMAKE_OPTIONS}}} are the options used to configure the build (see below). Without options, this results in the {{{release}}} build. (Use {{{--build mycustombuild}}} in the {{{fast-downward.py}}} script to select this build when running the planner.)

You can use a CMake GUI to set up all available options. To do so, on Unix-like systems replace the call to {{{cmake}}} by {{{ccmake}}} ({{{sudo apt install cmake-curses-gui}}}). On Windows, open the CMake GUI and enter the paths there.

Possible options to configure the build include:
  * {{{-DLIBRARY_BLIND_SEARCH_HEURISTIC_ENABLED=False}}}
    Switch off the blind heuristic.
    See {{{src/search/CMakeLists.txt}}} for other libraries.
  * {{{-DCMAKE_BUILD_TYPE=DEBUG}}}
    The only other build type is: {{{RELEASE}}} (default)
  * {{{-DCMAKE_C_COMPILER=/usr/bin/clang}}}, {{{-DCMAKE_CXX_COMPILER=/usr/bin/clang++}}}
    Force the use of `clang`/`clang++` (adjust paths as necessary).

You can also generate makefiles for other build systems (such as ninja) or generate project files for most IDEs:
  * {{{-GNinja}}}
    Use {{{ninja}}} instead of {{{make}}} in step 4.
  * {{{-G"NMake Makefiles"}}}
    Windows command line compile. Open the x86 developer shell for your compiler
    and then use {{{nmake}}} instead of {{{make}}} in step 4.
  * {{{-G"Visual Studio 15 2017"}}}
    This should generate a solution for Visual Studio 2017. Run this command in the command prompt with the environment variables loaded (i.e., execute the vsvarsall script).
  * {{{-G"XCode"}}}
    This should generate a project file for XCode.
  * Run {{{cmake}}} without parameters to see which generators are available on your system.

You can also change the compiler/linker and their options by setting the environment variables {{{CC}}}, {{{CXX}}}, {{{LINKER}}} and {{{CXXFLAGS}}}. These variables need to be set before running `./build.py` or executing `cmake` manually, so one drawback is that you cannot save such settings as build configurations in `build_config.py`. If you want to change these settings for an existing build, you must manually remove the build directory before rerunning `./build.py`.

Examples:
  * To compile with {{{clang}}} use:
  {{{#!highlight bash
CC=clang CXX=clang++ cmake ../../src}}}
  * Use full paths if the compiler is not found on the {{{PATH}}}, e.g., to force using the GNU compiler on macOS using !HomeBrew:

  /!\ Note that the following path is for GCC 4.8 which we no longer support. If you know the relevant path for a !HomeBrew version of GCC 10 or newer, please let us know.
  {{{#!highlight bash
CXX=/usr/local/Cellar/gcc48/4.8.3/bin/g++-4.8 CC=/usr/local/Cellar/gcc48/4.8.3/bin/g++-4.8 LINKER=/opt/local/bin/g++-mp-4.8 cmake ../../src}}}

  * The next example creates a build with the GNU compiler using !MacPorts:

  /!\ Note that the following path is for GCC 4.8 which we no longer support. If you know the relevant path for a !MacPorts version of GCC 10 or newer, please let us know.
  {{{#!highlight bash
CXX=/opt/local/bin/g++-mp-4.8 CC=/opt/local/bin/gcc-mp-4.8 LINKER=/opt/local/bin/g++-mp-4.8 cmake ../../src}}}

  * To abort compilation when the compiler emits a warning, set `CXXFLAGS="-Werror"`.

  * To force a 32-bit build on a 64-bit platform, set `CXXFLAGS="-m32"`. We recommend disabling the LP solver with 32-bit builds.

If you use a configuration often, it might make sense to add an alias for it in {{{build_configs.py}}}.

== Running the planner ==

For basic instructions on how to run the planner including examples, see PlannerUsage. The search component of the planner accepts a host of different options with widely differing behaviour. At the very least, you will want to choose a [[Doc/SearchAlgorithm|search algorithm]] with one or more [[Doc/Evaluator|evaluator specification]]s.

== Caveats ==

Please be aware of the following issues when working with the planner, '''especially if you want to use it for conducting scientific experiments''':

 1. We recommend using the [[Releases|latest release]]. If you are using the main branch instead, be aware that things can break or degrade with every commit. Typically they don't, but if they do, don't be surprised.
 1. There are '''known bugs''', especially with the translator component. To find out more, check out [[http://issues.fast-downward.org|our issue tracker]]. The planner has only really been tested with IPC domains, and even for those it does not work properly with all formulations of all domains. For more information, see the section on [[#Known_good_domains]] below.
 1. The '''search options''' are built with flexibility in mind, not ease of use. It is very easy to use option settings that look plausible, yet introduce significant inefficiencies. For example, an invocation like {{{
./fast-downward.py domain.pddl problem.pddl --search "lazy_greedy([ff()], preferred=[ff()])"}}} looks plausible, yet is hugely inefficient since it will compute the FF heuristic twice per state. See the examples on the PlannerUsage page to see how to call the planner properly. If in doubt, ask.

== Known good domains ==

There is a large collection of planning competition benchmarks at https://github.com/aibasel/downward-benchmarks, which includes all IPC domains (but not all ''formulations'' of all domains). The planner is somewhat sensitive to non-STRIPS language constructs and will choke on some valid PDDL inputs. Moreover, many of the heuristics do not support axioms or conditional effects. Even worse, sometimes the translator will introduce conditional effects even though the original PDDL input did not contain them. (We are working on this.)

We recommend that you use the sets of domains that are predefined in the [[https://github.com/aibasel/downward-benchmarks/blob/master/suites.py|suites.py]] script for your experiments. Here are the benchmark suites that we usually use in the two most common settings:

 * Inadmissible heuristics supporting conditional effects and axioms (e.g. context-enhanced additive, FF, additive, causal graph): {{{satisficing}}}
 * Admissible heuristics not supporting conditional effects and axioms (e.g. LM-Cut, iPDB): {{{optimal_strips}}}

After cloning the repo, you can list the domains in the respective suites by doing:

{{{#!highlight bash
./suites.py optimal_strips
}}}

You can use the resulting list of domains in your experiment scripts (see below).

== Experiments with Fast Downward  ==

The {{{Downward Lab}}} toolkit helps running Fast Downward experiments on large benchmark sets. &rarr; [[ScriptUsage|More information]]


================================================

= LP solver support =

Some configurations of the search component of Fast Downward, such as optimal cost partitioning for landmark heuristics, require a linear programming (LP) solver and will complain if the planner has not been built with support for such a solver. Running an LP configuration requires three steps, explained below:

 1. Installing one or more LP solvers.
 1. Building Fast Downward with LP support.
 1. Choosing a solver with command-line arguments.

== Step 1. Installing one or more LP solvers ==

Fast Downward uses a generic interface for accessing LP solvers and hence can be used together with different LP solvers. Currently, CPLEX and !SoPlex are supported. You can install one or both solvers without causing conflicts. Installation varies by solver and operating system.

=== Installing CPLEX on Linux/macOS ===

IBM offers a [[http://ibm.com/academic|free academic license]] that includes access to CPLEX.
Once you are registered, you find the software under Technology -> Data Science. Choose the right version and switch to HTTP download unless you have the IBM download manager installed. If you have problems using their website with Firefox, try Chrome instead. Execute the downloaded binary and follow the guided installation. If you want to install in a global location, you have to execute the installer as {{{root}}}.

After the installation, set the following environment variable.
The installer is for ILOG Studio, which contains more than just CPLEX, so the variable points to the subdirectory {{{/cplex}}} of the installation.
Adapt the path if you installed another version or did not install in the default location:
{{{
export cplex_DIR=/opt/ibm/ILOG/CPLEX_Studio2211/cplex
}}}

If you don't want to permanently modify your environment, you can also set these variables directly when calling CMake. The variable needs to be set when building Fast Downward's search component (Step 2.).

=== Installing CPLEX on Windows ===

Follow the Linux instructions to acquire a license and access the Windows-version of the CPLEX installer. Please install CPLEX into a directory without spaces.
For a silent installation, please consult: https://www.ibm.com/support/knowledgecenter/SSSA5P_12.9.0/ilog.odms.studio.help/Optimization_Studio/topics/td_silent_install.html

/!\ '''Important Note:''' Setting up environment variables might require using / instead of the more Windows-common \ to work correctly.

=== Installing SoPlex on Linux/macOS ===

!SoPlex is available under the Apache License from [[https://github.com/scipopt/soplex|Github]]. To be compatible with C++-20, we require a version of !SoPlex more recent than 6.0.3. At the time of this writing, 6.0.3 is the latest release, so we have to build from the unreleased version at the tip of the main branch.

You can install !SoPlex as follows (adapt the path if you install another version or want to use another location):
{{{

git clone https://github.com/scipopt/soplex.git
sudo apt install libgmp3-dev # The library is optional but important for SoPlex's performance
export soplex_DIR=/opt/soplex-6.0.3x
export CXXFLAGS="$CXXFLAGS -Wno-use-after-free" # Ignore compiler warnings about use-after-free
cmake -S soplex -B build
cmake --build build
cmake --install build --prefix $soplex_DIR
rm -rf soplex build
}}}

After installation, permanently set the environment variable {{{soplex_DIR}}} to the value you used in installation.

/!\ '''Note:''' Once a version including Salomé's fix is released, we can update this and can recommend the [[https://soplex.zib.de/index.php#download|SoPlex homepage]] for downloads instead.

=== Installing SoPlex on the grid in Basel  ===

To build !SoPlex on the grid, you should load a module with the GMP library and a compatible compiler module. The following setup should work:
{{{
module purge
module load GCC/11.3.0.lua
module load CMake/3.23.1-GCCcore-11.3.0.lua
module load Python/3.10.4-GCCcore-11.3.0.lua
module load GMP/6.2.1-GCCcore-11.3.0
}}}

Because the library is loaded from a module, it is not in a default directory, so change the CMake call to
{{{
cmake -S soplex -B build -DGMP_DIR="$EBROOTGMP"
}}}


== Step 2. Building Fast Downward with LP support ==

Once LP solvers are installed and the environment variables {{{cplex_DIR}}} and/or {{{soplex_DIR}}} are set up correctly, you can build Fast Downward's search component with LP support by calling {{{./build.py}}}. Remove your previous build first:
{{{
rm -rf builds
}}}

Fast Downward automatically includes an LP Solver in the build if it is needed and the solver is detected on the system. If you want to explicitly build without LP solvers that are installed on your system, use {{{./build.py release_no_lp}}}, or a [[ObtainingAndRunningFastDownward#Manual_Builds|manual build]] with the option {{{-DUSE_LP=NO}}}.


== Step 3. Choosing a solver with command-line arguments ==

Features that use an LP solver have a command-line option `lpsolver` to switch between different solver types. See [[http://issues.fast-downward.org/issue752|issue752]] and [[http://issues.fast-downward.org/issue1076|issue1076]] for a discussion of the relative performance of CPLEX and !SoPlex.

Note that !SoPlex is not a MIP solver, so using it for configurations that require integer variables will result in an error. Please use CPLEX for such cases.


== Troubleshooting ==

The LP-related libraries have a number of dependencies which might not be installed on your system. If for some reason one of the above steps fails, we would appreciate if you could attempt to troubleshoot it yourself.

If you get warnings about unresolved references with CPLEX, visit their [[http://www-01.ibm.com/support/docview.wss?uid=swg21399926|help pages]].

If you compiled Fast Downward on Windows (especially on !GitHub Actions) and cannot execute the binary in a new command line, then it might be unable to find a dynamically linked library. Use {{{dumpbin /dependents PATH\TO\DOWNWARD\BINARY}}} to list all required libraries and ensure that they can be found in your {{{PATH}}} variable.

If after troubleshooting you can get the LP package to work, please do let us know of your problem and its solution so that we can improve these instructions. If you still cannot get it to work, we may be able to provide some help, but note that the LP solvers are external packages not developed by us.
