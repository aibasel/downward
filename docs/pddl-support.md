# PDDL support of Fast Downward

On this page, we collect information regarding the subset of PDDL that
is supported by Fast Downward. So far, this is not an exhaustive list,
but we hope that it will become more comprehensive over time. If you
want to contribute information or have a question, please get in touch!
(See the contact information on the [home page](https://www.fast-downward.org).)

## General information

Fast Downward aims to support PDDL 2.2 level 1 plus the `:action-costs`
requirement from PDDL 3.1. For a definition of the various "levels" of
PDDL, see p. 63 the paper "PDDL2.1: An Extension to PDDL for Expressing
Temporal Planning Domains" by Maria Fox and Derek Long (JAIR 20:61-124,
2003).

This means that the following major parts of PDDL are unsupported:

-   All aspects of numerical planning. These are introduced at level 2
    of PDDL. Exception: some numerical features are part of the
    `:action-costs` requirement of PDDL 3.1, and these are supported by the
    planner.
-   All aspects of temporal planning. These are introduced at level 3 of
    PDDL and above.
-   Soft goals and preferences. These are introduced in PDDL 3.0.
-   Object fluents. These are introduced in PDDL 3.1.

Expressed positively, this means that the following features of PDDL are
supported beyond basic STRIPS, with some limitations mentioned below:

-   all ADL features such as quantified and conditional effects and
    negation, disjunction and quantification in conditions
-   axioms and derived predicates (introduced in PDDL 2.2)
-   action costs (introduced in PDDL 3.1)

## Limitations

-   **PDDL types:** `(either ...)` types are not supported
-   **conditional effects:** Not all heuristics support conditional
    effects. See [Evaluator](search/Evaluator.md) for details.
-   **axioms:** Not all heuristics support axioms. See
    [Evaluator](search/Evaluator.md) for details.
-   **universal conditions:** Universal conditions in preconditions,
    effect conditions and the goal are internally compiled into axioms
    by the planner. Therefore, heuristics that do not support axioms
    (see previous point) do not support universal conditions either.
-   **action costs:** Action costs must be non-negative integers (i.e.,
    not fractional), and each action may contain at most one effect
    affecting `(total-cost)`, which may not be part of conditional effects.
    These are the same restrictions that were in use for IPC 2008 and IPC 2011.

These limitations are somewhat likely to be lifted in the future, but progress
is slow.

## Bugs

Some features are supported in theory, but currently affected by bugs.
In addition to the issues with conditional effects mentioned above which
are somewhere between a bug and a missing feature, we are currently
aware of the following bugs:

-   <http://issues.fast-downward.org/issue215>: incorrect atoms
    (undeclared predicate, wrong arity, unknown object) and assignments
    (undeclared functions, wrong arity, referring to unknown objects, or
    unsupported value) may cause errors that are not correctly reported.

The above list might be outdated by the time you are reading this.
Follow the links to the issue tracker to be sure. If the list is not up
to date, it would be great if you could send us a note so that we can
remedy this. (See contact information on the [home page](https://www.fast-downward.org).)
