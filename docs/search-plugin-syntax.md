# Syntax for Search Plugins

This page explains the syntax for configuring the plugins for the search
component of the planner. 

## Meaning of the syntax documentation

All parameters can be specified by keyword or by position. Once a parameter is
specified by keyword, the rest of the parameters must be specified by keyword
too. Some parameters have default values and are optional. These parameters are
documented in the form `keyword = defaultvalue`.

Consider the following example:

    name(p, qs, r, s=v1, t=Enum1)

-   `p` (type_p): some explanation
-   `qs` (list of type_q): some explanation
-   `r` (type_r): some explanation
-   `s` (type_s): some explanation
-   `t` (Enum): some explanation
    -   Enum0: some explanation
    -   Enum1: some explanation
    -   Enum2: some explanation

Parameters `p`, `qs` and `r` are mandatory. `qs` is a list parameter. List
parameters have to be enclosed in square brackets. For example, let `h1`, `h2`,
`h3` be heuristic specifications, then `[h1, h3]` and `[h2]` are examples for
a list of heuristic specifications.

Parameters `s` and `t` are optional. `s` has the default value `v1` and `t` the
default value `Enum1`. `t` is an enumeration parameter and can only take the
values listed (here Enum0, Enum1, Enum2). These values may also be passed by
number, e.g. here `t=Enum1` and `t=1` are equivalent.

Some possible calls for this specification (with `X` and `Xi` having type_x):

-   `name(P, Q, R)`: `s` and `v` have their default values `v1` and `Enum1`
-   `name(P, [Q], R)`: equivalent to previous call
-   `name(P, [Q1, Q2], R, t=Enum2)`: `s` has its default value `v1`
-   `name(t=1, r=R, qs=[Q1, Q2], s=S1, p=P)` is equivalent to
    `name(P, [Q1, Q2], R, S1, 1)`

## Notes

-   Parameters of type `bool` are specified by strings `true` or `false`
-   not case-sensitive
-   To get positions and keywords, use

```
--help [Name]
```

## Integers

Parameters of type `int` can be specified as "infinity". This means that the
parameter will take the value `numeric_limits<int>::max()`, which is usually
equal to 2^31 - 1. If an `int` parameter value ends with "K", "M" or "G", the
value is multiplied by one thousand, one million or one billion, respectively.
For example, "bound=2K" is equivalent to "bound=2000".

## Strings

Parameters of type `string` can be specified in double quotes. Nested quotes
can be escaped as `\"`, backslashes as `\\`, and newlines as `\n`. E.g.,

    filename="C:\\some.file"

## Lists

List arguments have to be enclosed in square brackets now. E.g.,

    --evaluator "hff=ff()" --evaluator "hcea=cea()" \
    --search "lazy_greedy([hff, hcea], preferred=[hff, hcea])" \

## Enumerations

Enumeration arguments should be specified by name, e.g.:

    eager_greedy([h1,h2], cost_type=normal)

To get enumeration names (and more), run

    --help [Name]   //e.g. with Name=eager_greedy

## Variables

Often an object should be used for several purposes, e.g. a
[Heuristic](search/Evaluator.md) or a [LandmarkFactory](search/LandmarkFactory.md).
The most prevalent use case is a heuristic that is used for both the heuristic
estimates and for its preferred operators. In this case, one should define
a variable for the object. We currently only support variables for
[Heuristics](search/Evaluator.md) and [LandmarkFactories](search/LandmarkFactory.md)
but will extend the support for other feature types in the future.

Variables can be defined with

    let(name, definition, expression)

-   `name`: a variable name that should denote the feature
-   `definition`: an expression defining the value of the variable
-   `expression`: an expression defining any other feature.
    Occurrences of `name` in this expression may refer to the feature
    defined by `definition`.

### Variable Example

Suppose I want to run GBFS with the `landmark_sum` heuristic, and then run
another GBFS search with the `landmark_cost_partitioning` heuristic, using the
h^m landmarks without discovering the landmarks twice.

```
--search "let(lm, lm_hm(m=2), 
              iterated([lazy_greedy([landmark_sum(lm)]),
                        lazy_greedy([landmark_cost_partitioning(lm))]]))"
```

### Old-style Predefinitions

We still support but deprecate the use of "predefinitions" before the`--search`
option. They are internally converted to `let`-expressions.

The command lines

    --evaluator name=definition --search expression
    --landmarks name=definition --search expression

are both transformed to

    --search let(name, definition, expression)

## Conditional options

In some cases, it is useful to specify different options depending on
properties of the input file. For example, the LAMA 2011 configuration
makes use of this, adding an additional cost-ignoring search run at the
start for tasks with non-unit action costs.

### Example

    --if-unit-cost --evaluator "h1=ff()" --evaluator "h2=blind()" \
    --if-non-unit-cost --evaluator "h1=cea()" --evaluator "h2=lmcut()" \
    --always --search "eager_greedy([h1, h2])"

This conducts an eager greedy search with two heuristics. On unit-cost
tasks, it uses the FF heuristic and the blind heuristic. On other tasks,
it uses the context-enhanced additive heuristic and the LM-Cut
heuristic.

### Details

Options can be made conditional via *selectors* such as
`--if-unit-cost`. All options following a selector are only used if
the condition associated with the selector is true. (This really
includes **all** options, including ones like `--plan-file` that do
not affect the planning algorithm.) Each selector is in effect until it
is overridden by a new selector. The following selectors are available:

-   `--if-unit-cost`: the following options are only used for
    unit-cost planning tasks (i.e., tasks where all actions have cost 1,
    including the case where no action costs are specified at all)
-   `--if-non-unit-cost`: opposite of `--if-unit-cost`
-   `--always`: the following options are always used
