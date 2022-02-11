# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<https://github.com/aibasel/downward>) and the issue tracker
(<https://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Changes since the last release

- Add debugging methods to LP solver interface.
  <https://issues.fast-downward.org/issue960>
  You can now assign names to LP variables and constraints for easier
  debugging. Since this incurs a slight runtime penalty, we recommend
  against using this feature when running experiments.

- Support integer variables in linear programs.
  <https://issues.fast-downward.org/issue891>
  You can now use the LP solver interface to solve mixed integer programs.
  In particular, the operator-counting heuristics now have an option
  `use_integer_operator_counts` that improves the heuristic value by
  forcing operator counts to take integer values. Solving a MIP is NP-hard
  and usually takes much longer than solving the corresponding LP.

- For developers: move functionality used during search away from
  LandmarkGraph, making it constant after creation.
  <https://issues.fast-downward.org/issue988>
  <https://issues.fast-downward.org/issue1000>

- For developers: new state class
  <https://issues.fast-downward.org/issue348>
  We unified the classes GlobalState and State into a new class also called
  State. This removed a lot of code duplication and hacks from the code.
  A description of the new class can be found in the wiki:
  <https://www.fast-downward.org/ForDevelopers/Blog/A%20Deeper%20Look%20at%20States>

- For developers: introduce class for delete-relaxation based landmark
  factories and move usage of exploration object to subclasses of
  (abstract) landmark factory class.
  <https://issues.fast-downward.org/issue990>

- For users: We removed options from LandmarkFactories that were not relevant,
  renamed the option "no_orders" to "use_orders" and changed the
  reasonable_orders option to a Factory.
  <https://issues.fast-downward.org/issue995>
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
  <https://issues.fast-downward.org/issue940>

- For developers: We cleaned up the code of LandmarkGraph. Some of the public
  methods were renamed. This class will undergo further changes in the future.
  <https://issues.fast-downward.org/issue989>

- Debug builds with LP solvers vs. the _GLIBCXX_DEBUG flag
  <https://issues.fast-downward.org/issue982>
  Previously, we used the flag _GLIBCXX_DEBUG in debug builds for additional
  checks. This makes the binary incompatible with external libraries such as
  LP solvers. The flag is now disabled by default. If no LP solvers are present
  or LP solvers are disabled, it can be enabled by setting the CMake option
  USE_GLIBCXX_DEBUG. The build configurations debugnolp and releasenolp have
  been removed, and the build configuration glibcxx_debug has been added.

- For developers: decide on rules regarding software support and
  improve Github actions accordingly
  <https://issues.fast-downward.org/issue1003>

- For developers: add CPLEX support to our GitHub Actions for Windows
  <https://issues.fast-downward.org/issue1005>

- Fix a bug in the computation of RHW landmarks
  <https://issues.fast-downward.org/issue1004>

- Only build configurations defined in `build_configs.py` are loaded in the
  `build.py` script.
  <https://issues.fast-downward.org/issue1016>

- Replace size_t by int for abstract state hashes in PDB-related code
  <https://issues.fast-downward.org/issue1018>

- Integrate the pattern generation methods based on CEGAR
  <https://issues.fast-downward.org/issue1007>

- Integrate the random pattern generation methods
  <https://issues.fast-downward.org/issue1007>

- For developers: change public interface of generation of random ints and
  doubles in the RandomNumberGenerator class
  <https://issues.fast-downward.org/issue1026>

- For developers: we separate the functionality of landmarks from the
  functionality of landmark nodes by introducing a new Landmark class
  <https://issues.fast-downward.org/issue999>

- For developers: use RandomNumberGenerator class in VariableOrderFinder
  <https://issues.fast-downward.org/issue1032>

- For users: the driver now finds domain files <taskfile>-domain.<ext>
  for task files called <taskfile>.<ext>
  <https://issues.fast-downward.org/issue1033>

- For users: the build system now prefers compilers cc/c++ found on the path
  over gcc/g++. As before, environment variables CC/CXX can be used to
  override this choice.
  <https://issues.fast-downward.org/issue1031>

- For developers: Add option to use a local (configurable) logger instead of
  the global one.
  <https://issues.fast-downward.org/issue964>
  Classes which want to configure the logger (currently only the
  verbosity level can be configured) should now use the facilities
  add_log_options_to_parser and get_log_from_options to obtain their
  local log object.
- Fix a failing assertion in the landmark factory RHW triggered by 
  unsolvable tasks. 
  <https://issues.fast-downward.org/issue467>

- Remove a failing assertion in the lm_zg landmark factory. The issue was 
  triggered by an overly optimistic approximation of relaxed reachability in 
  the presence of conditional effects. While the assertion failed, this did
  not affect soundness of the other places where the same function is used. 
  Nevertheless, we changed the approximation to become stricter which can 
  lead to lm_rhw, lm_zg, and lm_exhaust finding more landmarks and landmark
  orderings.
  <https://issues.fast-downward.org/issue1041>

- Fix a bug where the Zhu/Givan landmark factory lead to a crash on relaxed
  unsolvable tasks due to returning an empty landmark graph.
  <https://issues.fast-downward.org/issue998>

- Delete-relation constraints can now be used in the operator-counting
  framework. The constraints defined by Imai and Fukunaga (JAIR 2015) encode
  different relaxations of the delete-relaxation heuristic.
  For details, see our documentation.
  <https://www.fast-downward.org/Doc/ConstraintGenerator#Delete_relaxation_constraints>
  Additionally, we fixed a bug which induced inadmissible
  heuristic values when using CPLEX for optimal planning with large action
  costs and/or long plans (only operator-counting heuristics with integer
  variables and tasks with total costs starting from 10'000 were affected).
  <https://issues.fast-downward.org/issue983>

- The landmark factories can handle cycles of natural orderings without
  crashing. Since the planning task is unsolvable in these cases, they 
  signal this by clearing the first achievers of the involved landmarks.
  <https://issues.fast-downward.org/issue937>

- New LimitedPruning class replaces previous limitation options of
  individual pruning methods
  <http://issues.fast-downward.org/issue1042>
  A previous command line option using this feature, such as
  --search "astar(lmcut(),pruning=atom_centric_stubborn_sets(min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000))"
  is now changed to
  --search "astar(lmcut(),pruning=limited_pruning(pruning=atom_centric_stubborn_sets(),min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000))


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
  <https://www.fast-downward.org/QuickStart> for more information.

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
  <https://issues.fast-downward.org/issue453>
  Derived predicates that were only needed in negated form and
  cyclically depended on other derived predicates could be computed
  incorrectly.

- Integrate new pruning method `atom_centric_stubborn_sets`.
  <https://issues.fast-downward.org/issue781>
  We merged the code for the SoCS 2020 paper "An Atom-Centric
  Perspective on Stubborn Sets"
  (<https://ai.dmi.unibas.ch/papers/roeger-et-al-socs2020.pdf>). See
  <https://www.fast-downward.org/Doc/PruningMethod>.

- Remove deprecated merge strategy aliases `merge_linear` and `merge_dfp`.
  The deprecated merge strategy aliases `merge_linear` for linear
  merge strategies and `merge_dfp` for the DFP merge strategy are no
  longer available. See https://www.fast-downward.org/Doc/MergeStrategy
  for equivalent command line options to use these merge strategies.

- For developers: use global logging mechanism for all output.
  <https://issues.fast-downward.org/issue963>
  All output of the planner is now handled by a global logging
  mechanism, which prefaces printed lines with time and memory
  information. For developers, this means that output should no longer
  be passed to `cout` but to `utils::g_log`. Further changes to
  logging are in the works.

- For developers: store enum options as enums (not ints) in Options objects.
  <https://issues.fast-downward.org/issue962>

- For developers: allow creating Timers in stopped state.
  <https://issues.fast-downward.org/issue965>

## Fast Downward 19.12

Released on December 20, 2019.

Highlights:

- Fast Downward no longer supports Python 2.7, which reaches its end
  of support on January 1, 2020. The minimum supported Python version
  is now 3.6.

- Fast Downward now supports the SoPlex LP solver.

Details:

- general: raise minimum supported Python version to 3.6
  <https://issues.fast-downward.org/issue939>
  Fast Downward now requires Python 3.6 or newer; support for Python 2.7 and
  Python 3.2-3.5 has been dropped. The main reason for this change is Python 2
  reaching its end of support on January 1, 2020. See
  https://python3statement.org/ for more background.

- LP solver: add support for the solver [SoPlex](https://soplex.zib.de/)
  <https://issues.fast-downward.org/issue752>
  The relative performance of CPLEX and SoPlex depends on the domain and
  configuration with each solver outperforming the other in some cases.
  See the issue for a more detailed discussion of performance.

- LP solver: add support for version 12.9 of CPLEX
  <https://issues.fast-downward.org/issue925>
  Older versions are still supported but we recommend using 12.9.
  In our experiments, we saw performance differences between version
  12.8 and 12.9, as well as between using static and dynamic linking.
  However, on average there was no significant advantage for any
  configuration. See the issue for details.

- LP solver: update build instructions of the open solver interface
  <https://issues.fast-downward.org/issue752>
  <https://issues.fast-downward.org/issue925>
  The way we recommend building OSI now links dynamically against the
  solvers and uses zlib. If your existing OSI installation stops
  working, try installing zlib (sudo apt install zlib1g-dev) or
  re-install OSI (https://www.fast-downward.org/LPBuildInstructions).

- merge-and-shrink: remove trivial factors
  <https://issues.fast-downward.org/issue914>
  When the merge-and-shrink computation terminates with several factors
  (due to using a time limit), only keep those factors that are
  non-trivial, i.e., which have a non-goal state or which represent a
  non-total function.

- tests: use pytest for running most tests
  <https://issues.fast-downward.org/issue935>
  <https://issues.fast-downward.org/issue936>

- tests: test Python code with all supported Python versions using tox
  <https://issues.fast-downward.org/issue930>

- tests: adjust style of Python code as suggested by flake8 and add this style
  check to the continuous integration test suite
  <https://issues.fast-downward.org/issue929>
  <https://issues.fast-downward.org/issue931>
  <https://issues.fast-downward.org/issue934>

- scripts: move Stone Soup generator scripts to separate repository at
  https://github.com/aibasel/stonesoup.
  <https://issues.fast-downward.org/issue932>

## Fast Downward 19.06

Released on June 11, 2019.
First time-based release.
