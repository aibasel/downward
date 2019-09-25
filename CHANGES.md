# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<http://hg.fast-downward.org>) and the issue tracker
(<http://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Changes since the last release

- LP solver: added support for the solver [SoPlex](https://soplex.zib.de/)
  <http://issues.fast-downward.org/issue752>
  The relative performance of CPLEX and SoPlex depends on the domain and
  configuration with each solver outperforming the other in some cases.
  See the issue for a more detailed discussion of performance.

- When the merge-and-shrink computation terminates with several factors (due to
  using a time limit), only keep those factors that are non-trivial, i.e.,
  which consist of at least a goal state or which represent a non-total
  function. <http://issues.fast-downward.org/issue914>

## Fast Downward 19.06

Released on June 11, 2019.
First time-based release.
