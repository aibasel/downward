# Usage

Fast Downward is a domain-independent classical planning system that consists of two main components:

-   `translate` translates a PDDL input task into a SAS+ tasks. 
-   `search` perfoms a search on a SAS+ task to find a plan.

To run Fast Downward, use the `fast-downward.py` driver script.
We will here give a short introduction into the basic usage of the three main components. 

However, the driver script has many additional options to do things like running portfolios, using a non-standard build and various other
things. To see the complete list of options, run

    ./fast-downward.py --help

If you want to run any of the planners based on Fast Downward that
participated in IPC 2011, please also check the page on
[IPC planners](ipc-planners.md).

## Translate component
To translate a PDDL input task into a SAS+ task without performing a search run the following:

    ./fast-downward.py --translate [<path-to-domain>/domain.pddl] <path-to-problem>/problem.pddl

Note:

-   Giving the domain file as input is optional. 
    In case no domain file is specified, the component will search for a file called `domain.pddl` located in `<path-to-problem>` directory. 
-   The paths to the domain/task files can be absolute, relative or contain path variables.
-   Translator component options can be specified after the input files following the `--translator-options` flag.

## Search component
To run a search on a PDDL input task run the following:

    ./fast-downward.py [<path-to-domain>/domain.pddl] <path-to-problem>/problem.pddl --search "<some-search-algorithm>(<some-evaluator>)"

Note:

-   The PDDL task is translated into a SAS+ task internally without the need to explicitel call the `translate` component.
    Instead of a PDDL task a SAS+ translator output file could be given as input. In this case the translation step is omitted.
-   Giving the domain file as input is optional. 
    In case no domain file is specified, the component will search for a file called `domain.pddl` located in the same folder as the task file. 
-   The paths to the domain/task files can be absolute, relative or contain path variables.
-   `<some-search-algorithm>` can be any of the [search algorithms](search/SearchAlgorithm.md) (e.g. `astar`)and `<some-evaluator>` can be one or more [evaluator specifications](search/Evaluator.md) (e.g. `lmcut()`).
-   Search component options can be specified anywhere after the input files. Search component options following translator component options need to first be escaped with the `--search-options` flag.

### Caveats

The search options are built with flexibility in mind, not ease of
use. It is very easy to use option settings that look plausible, yet
introduce significant inefficiencies. For example, an invocation like

    ./fast-downward.py <path-to-problem>/problem.pddl --search "lazy_greedy([ff()], preferred=[ff()])"

looks plausible, yet is hugely inefficient since it will compute the FF
heuristic twice per state. To circumvent this a `let`-expression could be used (see [here](search-plugin-syntax.md#variables_as_parameters)):

    ./fast-downward.py <path-to-problem>/problem.pddl --search "let(hff, ff(), lazy_greedy([hff], preferred=[hff]))"

## Validating plans
To validate a plan found by some search algorithm using [VAL](https://github.com/KCL-Planning/VAL) run the following:

    ./fast-downward.py --validate [<path-to-domain>/domain.pddl] <path-to-problem>/problem.pddl --search "<some-search-algorithm>(<some-evaluator>)"

Note:

-   [VAL](https://github.com/KCL-Planning/VAL) must be available locally and added to the PATH (detailed steps [here](https://github.com/aibasel/downward/blob/main/BUILD.md#optional-plan-validator)).
-   The search algorithm must be specified (see [above](#search_component)).

## Exit codes

The driver exits with 0 if no errors are encountered. Otherwise, it
returns the exit code of the first component that failed (cf. [documentation of
exit codes](exit-codes.md)).


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


## 64-bit mode

Older planner versions built the planner in 32-bit mode by default
because of lower memory consumption. As part of the meta issue
[issue213](http://issues.fast-downward.org/issue213) we
decreased the memory consumption of 64-bit builds to the point where
there should be no difference between 32- and 64-bit builds for most
configurations. Therefore, we use the native bitwidth of the operating
system since January 2019.

## LP support

Features that use an LP solver have a command-line option `lpsolver`
to switch between different solver types. See
[issue752](http://issues.fast-downward.org/issue752) and
[issue1076](http://issues.fast-downward.org/issue1076) for a
discussion of the relative performance of CPLEX and SoPlex.

Note that SoPlex is not a MIP solver, so using it for configurations
that require integer variables will result in an error. Please use CPLEX
for such cases.


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
