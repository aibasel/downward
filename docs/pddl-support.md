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

## Notes on syntax checks

There exist multiple PDDL definitions and they differ in some aspects, this
section describes how Fast Downward handles such cases.

- **empty conditions:** Like Fox and Long ("PDDL2.1: An Extension to PDDL for
  Expressing Temporal Planning Domains" by Maria Fox and Derek Long, JAIR
  20:61-124, 2003), Fast Downward allows empty conditions (i. e., preconditions
  and goals).
- **keyword "either":** As mentioned above, Fast Downward does not support
  `(either ...)` types, however it does not raise an error when `(either ...)`
  is used in the `:predicates` section. For example, defining `(somepredicate ?x -
  (either type1 type2))` is allowed (but ignored). Furthermore, type validity
  is not checked for the `:predicates` section.
- **empty list before type specification:** While most (if not all) PDDL
  specifications require at least one token before a type specification (e. g.
  `(someobject1 someobject2 - sometype)`), Fast Downward only prints a warning,
  instead of raising an error when nothing is given before a type
  specification (e. g. `(- sometype)`). The reason is that the woodworking
  domain from IPC 2008 and 2011 contain problems where this oddity occurs.
- **same name for predicate and type:** According to McDermott et al. ("PDDL -
  The Planning Domain Definition Language - Version 1.2" by Drew McDermott,
  Malik Ghallab, Adele Howe, Craig Knoblock, Ashwin Ram, Manuela Veloso, Daniel
  Weld, and David Wilkins, Yale Center for Computational Vision and Control,
  Technical Report CVC TR-98-003/DCS TR-1165, 1998) a type is essentially just
  a unary predicate. Thus, defining a new predicate with the same name as a
  type might lead to unexpected behaviour. Because of this, Fast Downward
  prints a warning if a predicate (of any arity) uses the same name as a type.
- **predefined type "number":** Fast Downward does not allow defining a type
  called "number" because this keyword is reserved.

## Bugs

Some features are supported in theory, but currently affected by bugs.
In addition to the issues with conditional effects mentioned above which
are somewhere between a bug and a missing feature, we keep track of known bugs
in our [issue tracker](http://issues.fast-downward.org/).

If you encounter a bug and cannot find it on the issue tracker, it would be
great if you could send us a note so that we can remedy this. (See contact
information on the [home page](https://www.fast-downward.org).)
