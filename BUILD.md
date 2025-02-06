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
Make sure that the build uses the library (check the output of CMake for `Found GMP`).

We require at least SoPlex 7.1.0, which can be built from source as follows (adapt the paths if you install a different version or want to use a different location):
```bash
sudo apt install libgmp3-dev
wget https://github.com/scipopt/soplex/archive/refs/tags/release-710.tar.gz -O - | tar -xz
cmake -S soplex-release-710 -B build
cmake --build build
export soplex_DIR=/opt/soplex-7.1.0
cmake --install build --prefix $soplex_DIR
rm -rf soplex-release-710 build
```

After installation, permanently set the environment variable `soplex_DIR` to the value you used during the installation.


### Optional: Plan Validator

You can validate the found plans by passing `--validate` (implied by `--debug`) to the planner if the [VAL plan validation software](https://github.com/KCL-Planning/VAL)
is installed on your system and the binary `validate` is on the `PATH`.

**Note:** VAL has a [bug](https://github.com/KCL-Planning/VAL/issues/48) that prevents it from correctly handling the IPC 18 data network domain.
You can install an older version, e.g., under Debian/Ubuntu:

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

Note that compiling from the terminal is only possible with the right environment. The easiest way to get such an environment is to use the `Developer PowerShell for VS 2019` or `Developer PowerShell`.

Alternatively, you can [create a Visual Studio Project](https://www.fast-downward.org/ForDevelopers/CMake#Custom_Builds), open it in Visual Studio and build from there. Visual Studio creates its binary files in subdirectories of the project that our driver script currently does not recognize. If you build with Visual Studio, you have to run the individual components of the planner yourself.

## Testing the build

To test your build use:

```bash
./fast-downward.py misc/tests/benchmarks/miconic/s1-0.pddl --search "astar(lmcut())"
```

To test the LP support use:
```bash
./fast-downward.py misc/tests/benchmarks/miconic/s1-0.pddl --search "astar(operatorcounting([lmcut_constraints()]))"
```

## Troubleshooting

* If you changed the build environment, delete the `builds` directory and rebuild.
* **Windows:** If you cannot execute the Fast Downward binary in a new command line, then it might be unable to find a dynamically linked library.
  Use `dumpbin /dependents PATH\TO\DOWNWARD\BINARY` to list all required libraries and ensure that they can be found in your `PATH` variable.
* **CPLEX:** After logging in at the IBM website, you find the Ilog studio software under Technology -> Data Science. Choose the right version and switch to HTTP download unless you have the IBM download manager installed. If you have problems using their website with Firefox, try Chrome instead. Execute the downloaded binary and follow the guided installation.
* **CPLEX:** If you get warnings about unresolved references with CPLEX, visit their [help pages](http://www-01.ibm.com/support/docview.wss?uid=swg21399926).
* **MacOS:** If your compiler doesn't find flex or bison when building VAL, your include directories might be in a non-standard location. In this case you probably have to specify where to look for includes and libraries in VAL's   
  Makefile (probably `/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr`).

