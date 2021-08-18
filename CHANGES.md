# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<https://github.com/aibasel/downward>) and the issue tracker
(<http://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Changes since the last release

- Add debugging methods to LP solver interface.
  <http://issues.fast-downward.org/issue960>
  You can now assign names to LP variables and constraints for easier
  debugging. Since this incurs a slight runtime penalty, we recommend
  against using this feature when running experiments.

- Support integer variables in linear programs.
  <http://issues.fast-downward.org/issue891>
  You can now use the LP solver interface to solve mixed integer programs.
  In particular, the operator-counting heuristics now have an option
  `use_integer_operator_counts` that improves the heuristic value by
  forcing operator counts to take integer values. Solving a MIP is NP-hard
  and usually takes much longer than solving the corresponding LP.

- For developers: move functionality used during search away from
  LandmarkGraph, making it constant after creation.
  <http://issues.fast-downward.org/issue988>
  <http://issues.fast-downward.org/issue1000>

- For developers: new state class
  <http://issues.fast-downward.org/issue348>
  We unified the classes GlobalState and State into a new class also called
  State. This removed a lot of code duplication and hacks from the code.
  A description of the new class can be found in the wiki:
  <http://www.fast-downward.org/ForDevelopers/Blog/A%20Deeper%20Look%20at%20States>

- For developers: introduce class for delete-relaxation based landmark
  factories and move usage of exploration object to subclasses of
  (abstract) landmark factory class.
  <http://issues.fast-downward.org/issue990>

- For users: We removed options from LandmarkFactories that were not relevant,
  renamed the option "no_orders" to "use_orders" and changed the
  reasonable_orders option to a Factory.
  <http://issues.fast-downward.org/issue995>
  Removed options:
  lm_exhaust: disjunctive_landmarks, conjunctive_landmarks, no_orders,
    reasonable_orders
  lm_hm: disjunctive_landmarks, only_causal_landmarks, no_orders,
    reasonable_orders
  lm_merged: disjunctive_landmarks, conjunctive_landmarks,
    only_causal_landmarks, no_orders, reasonable_orders
  lm_rhw: conjunctive_landmarks, no_orders, reasonable_orders
  lm_zg: disjunctive_landmarks, conjunctive_landmarks, only_causal_landmarks,
    no_orders, reasonable_orders
  Added options:
  lm_hm/lm_rhw/lm_zg: use_orders (negation of removed option "no_orders")
  New Factory "lm_reasonable_orders_hps": This factory approximates reasonable
  orders according to Hoffman, Porteus and Sebastia ("Ordered Landmarks in
  Planning", JAIR 2004) and is equivalent to the removed option
  "reasonable_orders", i.e. the command line argument
  --evaluator hlm=lmcount(lm_factory=lm_reasonable_orders_hps(lm_rhw()))
  is equivalent to the removed command line argument
  --evaluator hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true))

- For developers: add support for Github actions
  <http://issues.fast-downward.org/issue940>

- For developers: We cleaned up the code of LandmarkGraph. Some of the public
  methods were renamed. This class will undergo further changes in the future.
  <http://issues.fast-downward.org/issue989>

- Debug builds with LP solvers vs. the _GLIBCXX_DEBUG flag
  <http://issues.fast-downward.org/issue982>
  Previously, we used the flag _GLIBCXX_DEBUG in debug builds for additional
  checks. This makes the binary incompatible with external libraries such as
  LP solvers. The flag is now disabled by default. If no LP solvers are present
  or LP solvers are disabled, it can be enabled by setting the CMake option
  USE_GLIBCXX_DEBUG. The build configurations debugnolp and releasenolp have
  been removed, and the build configuration glibcxx_debug has been added.

- For developers: decide on rules regarding software support and
  improve Github actions accordingly
  <http://issues.fast-downward.org/issue1003>

- For developers: add CPLEX support to our GitHub Actions for Windows
  <http://issues.fast-downward.org/issue1005>

- Fix a bug in the computation of RHW landmarks
  <http://issues.fast-downward.org/issue1004>

- Only build configurations defined in `build_configs.py` are loaded in the
  `build.py` script.
  <http://issues.fast-downward.org/issue1016>

- Replace size_t by int for abstract state hashes in PDB-related code
  <http://issues.fast-downward.org/issue1018>

- Integrate the pattern generation methods based on CEGAR
  <http://issues.fast-downward.org/issue1007>

- Integrate the random pattern generation methods
  <http://issues.fast-downward.org/issue1007>

- For developers: change public interface of generation of random ints and
  doubles in the RandomNumberGenerator class
  <http://issues.fast-downward.org/issue1026>

- For developers: we separate the functionality of landmarks from the
  functionality of landmark nodes by introducing a new Landmark class
  <http://issues.fast-downward.org/issue999>

- For developers: use RandomNumberGenerator class in VariableOrderFinder
  <http://issues.fast-downward.org/issue1032>

- For users: the driver now finds domain files <taskfile>-domain.<ext>
  for task files called <taskfile>.<ext>
  <http://issues.fast-downward.org/issue1033>


## Fast Downward 20.06

Released on July 26, 2020.

Highlights:

- The Singularity and Docker distributions of the planner now include
  LP support using the SoPlex solver out of the box. Thank you to ZIB
  for their solver and for giving permission to include it in the
  release.

- The Vagrant distribution of the planner now includes LP support
  using the SoPlex and/or CPLEX solvers out of the box if they are
  made available when the virtual machine is first provisioned. See
  <http://www.fast-downward.org/QuickStart> for more information.

- A long-standing bug in the computation of derived predicates has
  been fixed. Thanks to everyone who provided bug reports for their
  help and for their patience!

- A new and much faster method for computing stubborn sets has been
  added to the planner.

- The deprecated merge strategy aliases `merge_linear` and `merge_dfp`
  have been removed.

Details:

- Fix crash of `--show-aliases` option of fast-downward.py.

- Fix incorrect computation of derived predicates.
  <http://issues.fast-downward.org/issue453>
  Derived predicates that were only needed in negated form and
  cyclically depended on other derived predicates could be computed
  incorrectly.

- Integrate new pruning method `atom_centric_stubborn_sets`.
  <http://issues.fast-downward.org/issue781>
  We merged the code for the SoCS 2020 paper "An Atom-Centric
  Perspective on Stubborn Sets"
  (<https://ai.dmi.unibas.ch/papers/roeger-et-al-socs2020.pdf>). See
  <http://www.fast-downward.org/Doc/PruningMethod>.

- Remove deprecated merge strategy aliases `merge_linear` and `merge_dfp`.
  The deprecated merge strategy aliases `merge_linear` for linear
  merge strategies and `merge_dfp` for the DFP merge strategy are no
  longer available. See http://www.fast-downward.org/Doc/MergeStrategy
  for equivalent command line options to use these merge strategies.

- For developers: use global logging mechanism for all output.
  <http://issues.fast-downward.org/issue963>
  All output of the planner is now handled by a global logging
  mechanism, which prefaces printed lines with time and memory
  information. For developers, this means that output should no longer
  be passed to `cout` but to `utils::g_log`. Further changes to
  logging are in the works.

- For developers: store enum options as enums (not ints) in Options objects.
  <http://issues.fast-downward.org/issue962>

- For developers: allow creating Timers in stopped state.
  <http://issues.fast-downward.org/issue965>

## Fast Downward 19.12

Released on December 20, 2019.

Highlights:

- Fast Downward no longer supports Python 2.7, which reaches its end
  of support on January 1, 2020. The minimum supported Python version
  is now 3.6.

- Fast Downward now supports the SoPlex LP solver.

Details:

- general: raise minimum supported Python version to 3.6
  <http://issues.fast-downward.org/issue939>
  Fast Downward now requires Python 3.6 or newer; support for Python 2.7 and
  Python 3.2-3.5 has been dropped. The main reason for this change is Python 2
  reaching its end of support on January 1, 2020. See
  https://python3statement.org/ for more background.

- LP solver: add support for the solver [SoPlex](https://soplex.zib.de/)
  <http://issues.fast-downward.org/issue752>
  The relative performance of CPLEX and SoPlex depends on the domain and
  configuration with each solver outperforming the other in some cases.
  See the issue for a more detailed discussion of performance.

- LP solver: add support for version 12.9 of CPLEX
  <http://issues.fast-downward.org/issue925>
  Older versions are still supported but we recommend using 12.9.
  In our experiments, we saw performance differences between version
  12.8 and 12.9, as well as between using static and dynamic linking.
  However, on average there was no significant advantage for any
  configuration. See the issue for details.

- LP solver: update build instructions of the open solver interface
  <http://issues.fast-downward.org/issue752>
  <http://issues.fast-downward.org/issue925>
  The way we recommend building OSI now links dynamically against the
  solvers and uses zlib. If your existing OSI installation stops
  working, try installing zlib (sudo apt install zlib1g-dev) or
  re-install OSI (http://www.fast-downward.org/LPBuildInstructions).

- merge-and-shrink: remove trivial factors
  <http://issues.fast-downward.org/issue914>
  When the merge-and-shrink computation terminates with several factors
  (due to using a time limit), only keep those factors that are
  non-trivial, i.e., which have a non-goal state or which represent a
  non-total function.

- tests: use pytest for running most tests
  <http://issues.fast-downward.org/issue935>
  <http://issues.fast-downward.org/issue936>

- tests: test Python code with all supported Python versions using tox
  <http://issues.fast-downward.org/issue930>

- tests: adjust style of Python code as suggested by flake8 and add this style
  check to the continuous integration test suite
  <http://issues.fast-downward.org/issue929>
  <http://issues.fast-downward.org/issue931>
  <http://issues.fast-downward.org/issue934>

- scripts: move Stone Soup generator scripts to separate repository at
  https://github.com/aibasel/stonesoup.
  <http://issues.fast-downward.org/issue932>

## Fast Downward 19.06

Released on June 11, 2019.
First time-based release.
