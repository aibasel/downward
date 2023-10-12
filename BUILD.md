## Dependencies
### Mandatory Dependencies

**Linux/MacOS:** you need a C++ compiler, CMake and GNU make.
  To run the planner, you also need Python 3.

  On Debian/Ubuntu, the following should install all these dependencies:
  ```
  sudo apt install cmake g++ make python3
  ```

**Windows:** install [Visual Studio >= 2017](https://visualstudio.microsoft.com/de/vs/older-downloads/),
[Python](https://www.python.org/downloads/windows/), and [CMake](http://www.cmake.org/download/).
During the installation of Visual Studio, the C++ compiler is not installed by default, but the IDE prompts you to install it when you create a new C++ project.


### Optional: Linear-Programming Solvers

Some planner configurations depend on an LP or MIP solver. We support CPLEX (commercial, [free academic license](http://ibm.com/academic)) and SoPlex (Apache License, no MIP support). You can install one or both solvers without causing conflicts.

Once LP solvers are installed and the environment variables `cplex_DIR` and/or `soplex_DIR` are set up correctly, Fast Downward automatically includes each solver detected on the system in the build.

#### Installing CPLEX

Obtain CPLEX and follow the guided installation. See [troubleshooting](#troubleshooting) if you have problems accessing the installer.
On Windows, install CPLEX into a directory without spaces.

After the installation, set the environment variable `cplex_DIR` to the subdirectory `/cplex` of the installation.
For example on Ubuntu:
```bash
export cplex_DIR=/opt/ibm/ILOG/CPLEX_Studio2211/cplex
```
Note that on Windows, setting up the environment variable might require using `/` instead of the more Windows-common `\`.


#### Installing SoPlex on Linux/macOS

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

After installation, permanently set the environment variable `soplex_DIR` to the value you used in installation.

**Note:** Once [support for C++-20](https://github.com/scipopt/soplex/pull/15) has been included in a SoPlex release, we can update this and can recommend the [SoPlex homepage](https://soplex.zib.de/index.php#download) for downloads instead.


### Optional: Plan Validator

You can validate the found plans by passing `--validate` to the planner if the [VAL plan validation software](https://github.com/KCL-Planning/VAL)
is installed on your system and the binary `validate` is on the `PATH`.

**Note:** VAL has a [bug](https://github.com/KCL-Planning/VAL/issues/48) that prevents it from correctly handling the IPC 18 data network domain.
You can install an older version, e.g. under Debian/Ubuntu:

```bash
sudo apt install g++ make flex bison
git clone https://github.com/KCL-Planning/VAL.git
cd VAL
git checkout a5565396007eee73ac36527fbf904142b3077c74
make clean  # Remove old binaries.
sed -i 's/-Werror //g' Makefile  # Ignore warnings.
make
```

Don't forget to add the resulting `validate` binary to your `PATH`.

## Compiling the planner

To build the planner, from the top-level directory run:

```bash
./build.py
```

This creates the default build `release` in the directory `builds`. For information on alternative builds (e.g. `debug`) and further options, call
`./build.py --help`. [Our website](https://www.fast-downward.org/ForDevelopers/CMake) has details on how to set up development builds.


### Compiling on Windows

Windows does not interpret the shebang in Python files, so you have to call `build.py` as `python3 build.py` (make sure `python3` is on your `PATH`). Also note that options are passed without `--`, e.g., `python3 build.py build=debug`.

Note that compiling from terminal is only possible with the right environment. The easiest way to get such an environment is to use the `Developer PowerShell for VS 2019` or `Developer PowerShell`.

Alternatively, you can [create a Visual Studio Project](https://www.fast-downward.org/ForDevelopers/CMake#Custom_Builds), open it in Visual Studio and build from there. Visual Studio creates its binary files in subdirectories of the project that our driver script currently does not recognize. If you build with Visual Studio, you have to run the individual components of the planner yourself.

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


Building VAL on macOS: If your compiler doesn't find flex or bison, your include directories might be in a non-standard location. In this case you probably have to specify where to look for includes and libraries in VAL's Makefile (probably `/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr`).

