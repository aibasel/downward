# Usage

To run Fast Downward, use the `fast-downward.py` driver script. At minimum, you
need to specify the PDDL input files and search options consisting of a [search
algorithm](search/SearchAlgorithm.md) with one or more [evaluator
specifications](search/Evaluator.md). The driver script has many options to do
things like running portfolios, running only the translation component of the
planner, using a non-standard build, running a plan validator and various other
things. To see the complete list of options, run

    ./fast-downward.py --help

If you want to run any of the planners based on Fast Downward that
participated in IPC 2011, please also check the page on
[IPC planners](ipc-planners.md).

## Caveats

The **search options** are built with flexibility in mind, not ease of
use. It is very easy to use option settings that look plausible, yet
introduce significant inefficiencies. For example, an invocation like

```
./fast-downward.py domain.pddl problem.pddl --search "lazy_greedy([ff()], preferred=[ff()])"
```

looks plausible, yet is hugely inefficient since it will compute the FF
heuristic twice per state. See the [examples](#examples) at the bottom of this
page to see how to call the planner properly. If in doubt, ask.

## Different builds

Different builds of Fast Downward (e.g. release vs. debug) are placed in
different directories by the build script. Hence, several builds can
coexist and `fast-downward.py` must be told which build to use. By default, the
`release` build is used, which is also the default build produced by
`build.py`.  To use a different build, pass `--build=<name>` to the driver
script. The parameter `--debug` is an alias for `--build=debug --validate`.

**Note on IDE projects (Visual Studio, XCode)**: You can use the CMake
build system to generate a project for you favourite IDE. These projects
are what CMake calls "multi-config generators", i.e., they are created
without fixing the build configuration. At build time, the IDE decides
whether to do a debug or release build and creates subdirectories in the
output folder. Use the full path to the binaries as the value of
`--build` (e.g., `--build=path/to/visual/studio/project/bin/Debug/`).

## Exit codes

The driver exits with 0 if no errors are encountered. Otherwise, it
returns the exit code of the first component that failed (cf. [documentation of
exit codes](exit-codes.md)).

## LP support

Features that use an LP solver have a command-line option `lpsolver`
to switch between different solver types. See
[issue752](http://issues.fast-downward.org/issue752) and
[issue1076](http://issues.fast-downward.org/issue1076) for a
discussion of the relative performance of CPLEX and SoPlex.

Note that SoPlex is not a MIP solver, so using it for configurations
that require integer variables will result in an error. Please use CPLEX
for such cases.

## Examples

### A* search

    # landmark-cut heuristic
     ./fast-downward.py domain.pddl task.pddl --search "astar(lmcut())"

    # iPDB heuristic with default settings
     ./fast-downward.py domain.pddl task.pddl --search "astar(ipdb())"

    # blind heuristic
     ./fast-downward.py domain.pddl task.pddl --search "astar(blind())"

### Lazy greedy best-first search with preferred operators and the queue alternation method

    ## using FF heuristic and context-enhanced additive heuristic (previously: "fFyY")
     ./fast-downward.py domain.pddl task.pddl \
        --search "let(hff, ff(), let(hcea, cea(), lazy_greedy([hff, hcea], preferred=[hff, hcea])))"
               

    ## using FF heuristic (previously: "fF")
     ./fast-downward.py domain.pddl task.pddl \
        --search "let(hff, ff(), lazy_greedy([hff], preferred=[hff]))"
               

    ## using context-enhanced additive heuristic (previously: "yY")
     ./fast-downward.py domain.pddl task.pddl \
        --search "let(hcea, cea(), lazy_greedy([hcea], preferred=[hcea]))"
               

### LAMA 2011

```
./fast-downward.py --alias seq-sat-lama-2011 domain.pddl task.pddl
```

runs the "LAMA 2011 configuration" of the planner. (Note that this is
not really the same as "LAMA 2011" as it participated at IPC 2011
because there have been bug fixes and other changes to the planner since
2011. See ["IPC planners"](ipc-planners.md) for more information.)
To find out which actual search options the LAMA 2011 configuration
corresponds to, check the source code of the `src/driver/aliases.py` module.


## 64-bit mode

Older planner versions built the planner in 32-bit mode by default
because of lower memory consumption. As part of the meta issue
[issue213](http://issues.fast-downward.org/issue213) we
decreased the memory consumption of 64-bit builds to the point where
there should be no difference between 32- and 64-bit builds for most
configurations. Therefore, we use the native bitwidth of the operating
system since January 2019.

## Other questions?

Please get in touch! See the [home page](https://www.fast-downward.org) for various
contact options.
