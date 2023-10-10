## Dependencies

- **Linux/MacOS:** you need a C++ compiler, CMake and GNU make.
  To run the planner, you will also need Python 3.

  On Debian/Ubuntu, the following should install all these dependencies:
  ```
  sudo apt install cmake g++ make python3
  ```
- **Windows:** install [Visual Studio >= 2017](https://visualstudio.microsoft.com/de/vs/older-downloads/),
[Python](https://www.python.org/downloads/windows/), and [CMake](http://www.cmake.org/download/).
During the installation of Visual Studio, the C++ compiler is not installed by default, but the IDE will prompt you to install it when you create a new C++ project.


### Linear-Programming configurations (optional)

Some planner configurations depend on an LP solver. 

================================================

# LP solver support

[[intro sentence above]]

Currently, CPLEX (commercial, [free academic license](http://ibm.com/academic)) and SoPlex (Apache License) are supported. You can install one or both solvers without causing conflicts.

## Installing CPLEX

Obtain CPLEX and follow the guided installation. See [troubleshooting](#troubleshooting) if you have problems accessing the installer.
On Windows, install CPLEX into a directory without spaces.

After the installation, set the environment variable `cplex_DIR` to the subdirectory `/cplex` of the installation.
For example on Ubuntu:
```bash
export cplex_DIR=/opt/ibm/ILOG/CPLEX_Studio2211/cplex
```
Note that on Windows, setting up the environment variable might require using `/` instead of the more Windows-common `\`.



## Installing SoPlex on Linux/macOS

**Important:**  The GNU Multiple Precision library (GMP) is critical for the performance of SoPlex but the build does not complain if it is not present.
Make sure that the build uses the library (check the output of cmake for `Found GMP`).

As of SoPlex 6.0.4, the release does not support C++-20, so we build  from the tip of the [Github main branch](https://github.com/scipopt/soplex) (adapt the path if you install another version or want to use another location):
```bash
sudo apt install libgmp3-dev
git clone https://github.com/scipopt/soplex.git
export soplex_DIR=/opt/soplex-6.0.4x
export CXXFLAGS="$CXXFLAGS -Wno-use-after-free" # Ignore compiler warnings about use-after-free
cmake -S soplex -B build
cmake --build build
cmake --install build --prefix $soplex_DIR
rm -rf soplex build
```

After installation, permanently set the environment variable `soplex_DIR}` to the value you used in installation.

**Note:** Once [support for C++-20](https://github.com/scipopt/soplex/pull/15) has been included in a SoPlex release, we can update this and can recommend the [SoPlex homepage](https://soplex.zib.de/index.php#download) for downloads instead.


Once LP solvers are installed and the environment variables {{{cplex_DIR}}} and/or {{{soplex_DIR}}} are set up correctly, Fast Downward automatically includes an LP Solver in the build if it is needed and the solver is detected on the system.








### Validating the computed plans

[good in principle]

You can validate the found plans by passing `--validate` to the planner. For this, the [VAL plan validation software](https://github.com/KCL-Planning/VAL)
needs to be installed on your system. [Here](SettingUpVal) you can find some instructions to help you set it up.

=> make sure to do something with SettingUpVal; include the necessary information here or point to it, but hopefully it's easier now in the first place

## Compiling the planner

To build the planner, from the top-level directory run:

```bash
./build.py
```

This will create our default build `release` in the directory `builds`. Other predefined build types are `debug`, `release_no_lp`, `glibcxx_debug` and `minimal`. Calling `./build.py --debug` will create a default debug build (equivalent to `debug`). You can pass make parameters to `./build.py`, e.g., `./build.py --debug -j4` will create a debug build using 4 threads for compilation (`-j4`), and `./build.py translate` will only build the translator component. (Because the translator is implemented in Python, "building" it just entails copying its source code into the build directory.) By default, `build.py` uses all cores for building the planner.

[Our website](https://www.fast-downward.org/ForDevelopers/CMake) has details on more complex builds.

### Compiling on macOS

Fast Downward should compile with the GNU C++ compiler and {{{clang}}} with the same instructions described above. In case your system compiler does not work for some reason, you will need to install the GNU compiler (if not already present) and tell CMake which compiler to use (see [[#Manual_Builds]]).

### Compiling on Windows

Windows does not interpret the shebang in Python files, so you have to call {{{build.py}}} as {{{python3 build.py}}} (make sure python3 is on your {{{PATH}}}). Also note that options are passed without {{{--}}}, e.g., {{{python3 build.py build=release_no_lp}}}.

Note that compiling from terminal is only possible with the right environment. The easiest way to get such an environment is to use the ``Developer Power''''''Shell for VS 2019`` or ``Developer Power''''''Shell.

Alternatively, you can create a Visual Studio Project (see [[#Manual_Builds]]), open it in Visual Studio and build from there. Visual Studio will create its binary files in subdirectories of the project that our driver script currently does not recognize. If you build with Visual Studio, you will have to run the individual components of the planner yourself.

## Running the planner

[[include a one-line example to test the build and a link to PlannerUsage]]


## Caveats
[[does not belong here]]

Please be aware of the following issues when working with the planner, '''especially if you want to use it for conducting scientific experiments''':
We recommend using the [[Releases|latest release]]. If you are using the main branch instead, be aware that things can break or degrade with every commit. Typically they don't, but if they do, don't be surprised.

=> quick start/README


## Known good domains
[[remove section, but keep info below in some form]]

=> add link to downward-benchmarks repo or some other way to do a minimal test of a build (e.g. using PDDL files included in the repository)
=> otherwise link to downward-benchmarks belongs to README or something similar

https://github.com/aibasel/downward-benchmarks

The {{{Downward Lab}}} toolkit helps running Fast Downward experiments on large benchmark sets. &rarr; [[ScriptUsage|More information]]
=> Perhaps also reference downward-lab.

=> TODO: What is with this ScriptUsage page?



# Troubleshooting

[[keep a troubleshooting section; perhaps include rm -rf build here; make this a generic troubleshooting section, not an LP-specific one]]

If you get warnings about unresolved references with CPLEX, visit their [[http://www-01.ibm.com/support/docview.wss?uid=swg21399926|help pages]].

If you compiled Fast Downward on Windows (especially on !GitHub Actions) and cannot execute the binary in a new command line, then it might be unable to find a dynamically linked library. Use {{{dumpbin /dependents PATH\TO\DOWNWARD\BINARY}}} to list all required libraries and ensure that they can be found in your {{{PATH}}} variable.
=> is this still current?

IBM offers a [free academic license](http://ibm.com/academic) that includes access to CPLEX.
Once you are registered, you find the software under Technology -> Data Science. Choose the right version and switch to HTTP download unless you have the IBM download manager installed. If you have problems using their website with Firefox, try Chrome instead. Execute the downloaded binary and follow the guided installation.
