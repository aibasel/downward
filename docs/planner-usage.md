# Usage

Fast Downward is a domain-independent classical planning system that consists of two main components:

-   a translation phase translates a PDDL input task into a SAS+ tasks. 
-   a search phase perfoms a search on a SAS+ task to find a plan.

To run Fast Downward, use the `fast-downward.py` driver script.
We will here give a short introduction into the basic usage of the main components. 

Note that the driver script has many additional options to do things like running portfolios, using a non-standard build and various other
things. To see the complete list of options, run

    ./fast-downward.py --help

If you want to run any of the planners based on Fast Downward that
participated in IPC 2011, please also check the page on
[IPC planners](ipc-planners.md).

## Translation component
To translate a PDDL input task into a SAS+ task without performing a search run the following:

    ./fast-downward.py --translate [<domain.pddl>] <task.pddl>

Note:

-   Giving the domain file as input is optional. 
    In case no domain file is specified, the component will search for a file called `domain.pddl` located in the same directory as the task file. 
-   Translator component options can be specified after the input files following the `--translator-options` flag.

## Search component
To run a search on a PDDL input task run the following:

    ./fast-downward.py [<domain.pddl>] <task.pddl> \
    --search "<some-search-algorithm>(<algorithm-parameters>)"

Note:

-   The PDDL task is translated into a SAS+ task internally without the need to explicitly call the `translate` component.
    Instead of a PDDL task a SAS+ translator output file could be given as input. In this case the translation step is omitted.
-   Giving the domain file as input is optional. 
    In case no domain file is specified, the component will search for a file called `domain.pddl` located in the same directory as the task file. 
-   `<some-search-algorithm>` can be any of the [search algorithms](search/SearchAlgorithm.md) with corresponding `<algorithm-parameters>` as input.
-   Search component options can be specified anywhere after the input files. 
    Search component options following translator component options need to first be escaped with the `--search-options` flag.

### Example

The following example is a recommended search algorithm configuration:
[A* algorithm](search/SearchAlgorithm/#a_search_eager) with the [LM-cut heuristic](search/Evaluator/#landmark-cut_heuristic), which can be run as follows:

    ./fast-downward.py [<domain.pddl>] <task.pddl> --search "astar(lmcut())"

As this is a common search configuration an alias (`seq-opt-lmcut`) exists that results in the same execution and can be used as follows:

    ./fast-downward.py --alias seq-opt-lmcut [<domain.pddl>] <task.pddl>

### Aliases

Many other common search configurations are also specified as aliases. To see the full list of aliases call:

    ./fast-downward.py --show-aliases

To find out which actual search options the aliases corresponds to, check the source code of the `src/driver/aliases.py` module.

For example to run the "LAMA 2011 configuration" of the planner, call:

    ./fast-downward.py --alias seq-sat-lama-2011 [<domain.pddl>] <task.pddl> 

Note:

-   This is not really the same as "LAMA 2011" as it participated at IPC 2011
    because there have been bug fixes and other changes to the planner since 2011.
    See ["IPC planners"](ipc-planners.md) for more information. 

### Caveats

The search options are built with flexibility in mind, not ease of
use. It is very easy to use option settings that look plausible, yet
introduce significant inefficiencies. For example, an invocation like

    ./fast-downward.py [<domain.pddl>] <task.pddl> \
    --search "lazy_greedy([ff()], preferred=[ff()])"

looks plausible, yet is hugely inefficient since it will compute the FF
heuristic twice per state. To circumvent this a [`let`-expression](search-plugin-syntax.md#variables_as_parameters) could be used:

    ./fast-downward.py [<domain.pddl>] <task.pddl> \
    --search "let(hff, ff(), lazy_greedy([hff], preferred=[hff]))"

## Validating plans
To validate a plan found by some search algorithm using [VAL](https://github.com/KCL-Planning/VAL) run the following:

    ./fast-downward.py --validate [<domain.pddl>] <task.pddl> \
    --search "<some-search-algorithm>(<algorithm-parameters>)"

Note:

-   [VAL](https://github.com/KCL-Planning/VAL) must be available locally and added to the PATH (see [Plan Validator](https://github.com/aibasel/downward/blob/main/BUILD.md#optional-plan-validator)).
-   The search algorithm must be specified (see [Search component](#search_component)).

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


## LP support

Features that use an LP solver have a command-line option `lpsolver`
to switch between different solver types. See
[issue752](http://issues.fast-downward.org/issue752) and
[issue1076](http://issues.fast-downward.org/issue1076) for a
discussion of the relative performance of CPLEX and SoPlex.

Note that SoPlex is not a MIP solver, so using it for configurations
that require integer variables will result in an error. Please use CPLEX
for such cases.
