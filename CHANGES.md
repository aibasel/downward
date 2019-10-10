# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<http://hg.fast-downward.org>) and the issue tracker
(<http://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Changes since the last release

- LP solver: updated build instructions of the open solver interface.
  <http://issues.fast-downward.org/issue752>
  <http://issues.fast-downward.org/issue925>
  The way we recommend building OSI now links dynamically against the
  solvers and uses zlib. If your existing OSI installation stops
  working, try installing zlib (sudo apt install zlib1g-dev) or
  re-install OSI (http://www.fast-downward.org/LPBuildInstructions).

- LP solver: added support for version 12.9 of CPLEX.
  <http://issues.fast-downward.org/issue925>
  Older versions are still supported but we recommend using 12.9.
  In our experiments, we saw performance differences between version
  12.8 and 12.9, as well as between using static and dynamic linking.
  However, on average there was no significant advantage for any
  configuration. See the issue for details.

- LP solver: added support for the solver [SoPlex](https://soplex.zib.de/)
  <http://issues.fast-downward.org/issue752>
  The relative performance of CPLEX and SoPlex depends on the domain and
  configuration with each solver outperforming the other in some cases.
  See the issue for a more detailed discussion of performance.

- When the merge-and-shrink computation terminates with several factors (due to
  using a time limit), only keep those factors that are non-trivial, i.e.,
  which have a non-goal state or which represent a non-total function.
  <http://issues.fast-downward.org/issue914>

- Test Python code with all supported Python versions using tox.
  <http://issues.fast-downward.org/issue930>

- Adjust style of Python code as suggested by flake8 and add this style
  check to the continuous integration test suite.
  <http://issues.fast-downward.org/issue931>
  <http://issues.fast-downward.org/issue934>

- Move Stone Soup generator scripts to separate repository at
  https://bitbucket.org/aibasel/stonesoup.
  <http://issues.fast-downward.org/issue932>

- Use pytest for testing standard configurations.
  <http://issues.fast-downward.org/issue935>

## Fast Downward 19.06

Released on June 11, 2019.
First time-based release.
