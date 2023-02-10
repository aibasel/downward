# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<https://github.com/aibasel/downward>) and the issue tracker
(<https://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Changes since the last release

- option parser: We implemented a new way of defining features and
  parsing them from the command line. The new parser now supports
  defining variables for features (heuristics and landmark graphs so
  far) within the option string. For example
    `let(h, lmcut(), astar(h))`.
  This change to the parser was an important stepping stone towards
  solving a more general problem about how components interact.
  Details of the new parser are described in a blog article. While
  working on this, we also improved the existing documentation of enum
  values.
  <https://www.fast-downward.org/ForDevelopers/Blog/TheNewOptionParser>
  <https://issues.fast-downward.org/issue1073>
  <https://issues.fast-downward.org/issue1040>

- landmarks: Refactor the computation of preferred operators in the
  `lmcount` heuristic. The change affects configurations based on
  LAMA that use preferred operators. While the semantics of the code did
  not change, the new version is slightly faster and can solve more 
  tasks and/or improves plan quality in an anytime configuration within 
  the same time limit.
  <https://issues.fast-downward.org/issue1070>

## Fast Downward 22.12

Released on December 15, 2022.

Highlights:

- We now test more recent versions of Ubuntu Linux (22.04 and 20.04),
  macOS (11 and 12) and Python (3.8 and 3.10).

- Most search algorithms are now faster. We fixed a performance
  problem related to state pruning, which also affected search
  configurations that did not explicitly select a pruning method.

- All landmark factories now respect action costs. Previously, this
  was only the case when using admissible landmark heuristic or when
  using the `lm_rhw` landmark factory. Note that ignoring action costs
  (i.e., the old behaviour for landmark factories other than `lm_rhw`)
  often finds plans faster and is still possible with the
  `adapt_costs` transformation.

Details:

- driver: Planner time is now logged in a consistent format.
  Previously, it would sometimes be logged in scientific notation.

- driver, for developers: Skip __pycache__ directory when collecting
  portfolios.
  <https://issues.fast-downward.org/issue1057>

- translator: Allow importing pddl_parser without parsing arguments
  from command line.
  <https://issues.fast-downward.org/issue1068>

- pruning methods: Fix a performance regression caused by spending too
  much time measuring elapsed time. This is now only done at verbosity
  level `verbose` or higher. Verbosity level parameter added to all
  pruning methods.
  <https://issues.fast-downward.org/issue1058>
  Note that most search algorithms in Fast Downward always use a
  pruning method (a trivial method pruning nothing is used by default)
  and were therefore affected by this performance problem.

- pruning methods, for developers: We cleaned up the internal
  structure of stubborn set pruning.
  <https://issues.fast-downward.org/issue1059>

- landmarks: All landmark factories are now sensitive to action costs.
  <https://issues.fast-downward.org/issue1009>
  When using the `lmcount` heuristic in inadmissible mode (option
  `admissible=false`), previously only the `lm_rhw` landmark factory
  considered action costs. Now, all landmark factories do. (This was
  already the case with `admissible=true`.)
  Experiments show that ignoring action costs is often beneficial when
  we are more interested in planner speed or coverage than plan
  quality. This can be achieved by using the option
  `transform=adapt_costs(ONE)`.

- landmarks: Reduce verbosity of h^m landmarks.
  The `lm_hm` landmark factory is now less verbose by default. Use
  verbosity level `verbose` or higher to enable the previous output.

- infrastructure: Update tested OS versions and clang-tidy version.
  <https://issues.fast-downward.org/issue1067>
  - The tested Ubuntu versions are now 22.04 and 20.04.
  - The tested macOS versions are now macOS 11 and macOS 12.
  - The tested Windows version remains Windows 10.
  - We now test Python 3.10 (Ubuntu 22.04, macOS 12)
    and Python 3.8 (Ubuntu 20.04, macOS11, Windows 10).
  - We now use clang-tidy-12.
   See `README.md` for details.

- infrastructure: Update delete-artifact version number in GitHub
  action, update zlib version in Windows build.

## Fast Downward 22.06.1

Released on September 15, 2022.

This is a bugfix release fixing two serious bugs in Fast Downward
22.06:

- Driver configurations relying on certain kinds of time limits (using
  the `--overall-time-limit` option or portfolios) crashed when using
  Python 3.10.
  <https://issues.fast-downward.org/issue1064>

- Using post-hoc optimization constraints (`pho_constraints`) caused
  crashes (segmentation faults) or other undefined behavior.
  <https://issues.fast-downward.org/issue1061>


## Fast Downward 22.06

Released on June 16, 2022.

Highlights:

- We fixed a bug in the translator component that could lead to
  incorrect behavior in tasks where predicates are mentioned in the
  goal that are not modified by any actions.

- Various speed improvements to landmark factories. This is part of a
  larger ongoing clean-up of the landmark code.

- More informative output, and more control over the output. The
  driver now prints the total runtime of all components. For many
  planner components, including all heuristics, the verbosity level
  can now be configured individually.

Details:

- translator: Fix a bug where the translator would not check goal
  conditions on predicates that are not modified by actions.
  <https://issues.fast-downward.org/issue1055>

- driver: Print overall planner resource limits and overall planner
  runtime on Linux and macOS systems.
  <https://issues.fast-downward.org/issue1056>

- logging: verbosity option for all evaluators
  <https://issues.fast-downward.org/issue921>
  All evaluators and heuristics now have their own configurable logger
  and no longer use g_log. These loggers have a verbosity option,
  which allows choosing between silent, normal, verbose and debug for
  all instances of evaluators created on the command line.

- landmarks: Speed up landmark generation time by 10-20% for `lm_rhw`,
  `lm_zg`, and `lm_exhaust` by avoiding unnecessary computations in
  the landmark exploration.
  <https://issues.fast-downward.org/issue1044>

- landmarks: Speed up landmark generation time by 5-15% for `lm_rhw`,
  `lm_zg`, and `lm_exhaust` by computing reachability in the landmark
  exploration as boolean information instead of (unused) integer
  cost/level information.
  <https://issues.fast-downward.org/issue1045>

- landmarks: Improve landmark dead-end detection so that relevant
  static information is only computed once, instead of at every state
  evaluation.
  <https://issues.fast-downward.org/issue1049>

- infrastructure: Upgrade GitHub Actions to Windows Server 2019
  (Visual Studio Enterprise 2019) and Windows Server 2022 (Visual
  Studio Enterprise 2022). Remove Windows Server 2016, because GitHub
  Actions no longer support it.
  <https://issues.fast-downward.org/issue1054>

- infrastructure: Run GitHub Actions only for the following branches:
  `main`, `issue*`, `release-*`.
  <https://issues.fast-downward.org/issue1027>

## Fast Downward 21.12

Released on February 16, 2022.

Highlights:

- Fast Downward now has a logo!

- We added new methods for generating patterns and pattern collections
  based on counterexample-guided abstraction refinement and a new
  highly random method for generating individual patterns based on the
  causal graph. These methods are due to Rovner et al. (ICAPS 2019).

- The operator-counting heuristic now has an option to use integer
  operator counts rather than real-valued operator counts. This makes
  the heuristic more accurate at a vastly increased computational cost
  (not generally recommended, but very useful for targeted
  experiments). We added a new constraint generator for Imai and
  Fukunaga's delete relaxation constraints (JAIR 2015). With the right
  option settings, the operator-counting heuristic with this new
  constraint generator results in the optimal delete relaxation
  heuristic h+.

- Pruning methods now have a different interface. The mechanism to
  disable pruning automatically after a number of expansions that
  resulted in little pruning is now implemented as its own pruning
  method that wraps another pruning method. Be careful that the old
  syntax is still accepted by the planner, but the options that limit
  pruning are ignored. (This is due to an option parser bug; a fix is
  in the works.)

- In our ongoing efforts to improve the landmark code, the landmark
  factories and landmark-count heuristic received a major overhaul. We
  removed irrelevant options for landmark factories, decoupled the
  computation of reasonable orders from landmark generation, made many
  internal code and data structure changes to make the code nicer to
  work with and fixed several long-standing bugs.

- All pattern generators and pattern collection generators now have
  controllable verbosity. Similar changes to other components of the
  planner are planned. This is part of a general effort to make
  logging more configurable.

- For developers: The internal representation of states has been
  overhauled, resolving the confusion between the previous classes
  `GlobalState` and `State`.

Details:

- new Fast Downward logo
  <https://issues.fast-downward.org/issue1024>
  You can see the logo in the README file on
  <https://github.com/aibasel/downward>. Check out
  <https://www.fast-downward.org/ForDevelopers/Blog/LogoDesigns> for
  alternative suggestions including the ever so popular "truck falling
  down the hill" logo.

- fast-downward.py main script: The script now automatically finds domain
  files `<taskfile>-domain.<ext>` for task files called `<taskfile>.<ext>`
  <https://issues.fast-downward.org/issue1033>

- pdbs: Integrate the Rovner et al. pattern generation methods based
  on CEGAR.
  <https://issues.fast-downward.org/issue1007>

- pdbs: Integrate the Rovner et al. random pattern generation methods.
  <https://issues.fast-downward.org/issue1008>

- pdbs: All pattern (collection) generators now have an option
  `verbosity` to set the desired level of output.
  <https://issues.fast-downward.org/issue1043>
  Internally, generators now use their own logger rather than g_log.

- pdbs, for developers: Replace size_t by int for abstract state
  hashes in PDB-related code.
  <https://issues.fast-downward.org/issue1018>

- LP/IP: Support integer variables in linear programs.
  <https://issues.fast-downward.org/issue891>
  <https://issues.fast-downward.org/issue1048>
  You can now use the LP solver interface to solve mixed integer programs.
  In particular, the operator-counting heuristics now have an option
  `use_integer_operator_counts` that improves the heuristic value by
  forcing operator counts to take integer values. Solving a MIP is NP-hard
  and usually takes much longer than solving the corresponding LP.

- LP/IP: Delete-relation constraints can now be used in the
  operator-counting framework. The constraints defined by Imai and
  Fukunaga (JAIR 2015) encode different relaxations of the
  delete-relaxation heuristic. For details, see
  <https://www.fast-downward.org/Doc/ConstraintGenerator#Delete_relaxation_constraints>
  <https://issues.fast-downward.org/issue983>

- LP/IP: Fix a bug which induced inadmissible heuristic values when
  solving MIPs. This only occurred for operator-counting heuristics
  with integer variables and very large heuristic values (at least
  10,000).
  <https://issues.fast-downward.org/issue983>

- LP/IP, for developers: Add debugging methods to LP solver interface.
  <https://issues.fast-downward.org/issue960>
  You can now assign names to LP variables and constraints for easier
  debugging. Since this incurs a slight runtime penalty, we recommend
  against using this feature when running experiments.

- LP/IP, for developers: Debug builds with LP solvers vs. the
  `_GLIBCXX_DEBUG` flag.
  <https://issues.fast-downward.org/issue982>
  Previously, we used the flag `_GLIBCXX_DEBUG` in debug builds for
  additional checks. This makes the binary incompatible with external
  libraries such as LP solvers. The flag is now disabled by default.
  If no LP solvers are present or LP solvers are disabled, it can be
  enabled by setting the CMake option `USE_GLIBCXX_DEBUG`. The build
  configurations `debugnolp` and `releasenolp` have been removed, and
  the build configuration `glibcxx_debug` has been added.

- pruning: New `LimitedPruning` class replaces previous limitation
  options of individual pruning methods.
  <https://issues.fast-downward.org/issue1042>
  For example, the old command line
  `--search "astar(lmcut(),pruning=atom_centric_stubborn_sets(min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000))"`
  is now expressed as
  `--search "astar(lmcut(),pruning=limited_pruning(pruning=atom_centric_stubborn_sets(),min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000))`

- landmarks: Replace landmark factory option `reasonable_orders` by
  the new landmark factory `lm_reasonable_orders_hps`.
  <https://issues.fast-downward.org/issue995>
  For example, the old command line
  `--evaluator hlm=lmcount(lm_factory=lm_rhw(reasonable_orders=true))`
  is now expressed as
  `--evaluator hlm=lmcount(lm_factory=lm_reasonable_orders_hps(lm_rhw()))`

- landmarks: Replace landmark factory option `no_orders` by the new
  option `use_orders` with the opposite meaning.
  <https://issues.fast-downward.org/issue995>

- landmarks: Removed landmark factory options that have no effect.
  <https://issues.fast-downward.org/issue995>
  Removed options:
  `lm_exhaust`: `disjunctive_landmarks`, `conjunctive_landmarks`
  `lm_hm`: `disjunctive_landmarks`, `only_causal_landmarks`
  `lm_merged`: `disjunctive_landmarks`, `conjunctive_landmarks`,
    `only_causal_landmarks`
  `lm_rhw`: `conjunctive_landmarks`, `reasonable_orders`
  `lm_zg`: `disjunctive_landmarks`, `conjunctive_landmarks`,
    `only_causal_landmarks`

- landmarks: Fix a bug where `lm_rhw` would compute wrong
  greedy-necessary orderings in certain cases.
  <https://issues.fast-downward.org/issue1004>

- landmarks: Fix a bug where the `lm_rhw`, `lm_zg` and `lm_exhaust`
  landmark factories used an overly optimistic approximation of
  relaxed reachability for planning tasks with conditional effects.
  This change can lead to more generated landmarks and landmark
  orderings in such tasks.
  <https://issues.fast-downward.org/issue1041>

- landmarks: Fix a bug where the Zhu/Givan landmark factory caused a
  crash on relaxed unsolvable tasks due to an empty landmark graph.
  <https://issues.fast-downward.org/issue998>

- landmarks: Fix a bug where cycles of natural orderings resulted in
  crashes in the landmark factories. This could only happen in
  unsolvable planning tasks.
  <https://issues.fast-downward.org/issue937>

- landmarks: Fix a failing assertion in the `lm_rhw` landmark factory
  triggered by certain unsolvable tasks.
  <https://issues.fast-downward.org/issue467>

- landmarks, for developers: Clean up the code of `LandmarkGraph`.
  Some of the public methods were renamed. This class will undergo
  further changes in the future.
  <https://issues.fast-downward.org/issue989>

- landmarks, for developers: Separate the functionality of landmarks
  from the functionality of landmark nodes by introducing a new
  `Landmark` class.
  <https://issues.fast-downward.org/issue999>

- landmarks, for developers: Move functionality used during search
  away from `LandmarkGraph`, making the landmark graph constant after
  creation.
  <https://issues.fast-downward.org/issue988>
  <https://issues.fast-downward.org/issue1000>

- landmarks, for developers: Introduce new class
  `LandmarkFactoryRelaxation` for landmark factories based on delete
  relaxation. Move usage of exploration object to subclasses of the
  landmark factory base class.
  <https://issues.fast-downward.org/issue990>

- build: The build system now prefers compilers `cc`/`c++` found on
  the path over `gcc`/`g++`. As before, environment variables
  `CC`/`CXX` can be used to override this choice.
  <https://issues.fast-downward.org/issue1031>

- build: Only build configurations defined in `build_configs.py` are
  loaded in the `build.py` script.
  <https://issues.fast-downward.org/issue1016>

- for developers: Add option to use a local (configurable) logger instead of
  the global one.
  <https://issues.fast-downward.org/issue964>
  Classes which want to configure the logger (currently only the
  verbosity level can be configured) should now use the functions
  `add_log_options_to_parser` and `get_log_from_options` to obtain their
  local log object.

- for developers: Unify the `State` and `GlobalState` classes.
  <https://issues.fast-downward.org/issue348>
  We unified the classes `GlobalState` and `State` into a new class
  also called `State`. This removed a lot of code duplication and hacks
  from the code. A description of the new class can be found in the wiki:
  <https://www.fast-downward.org/ForDevelopers/Blog/ADeeperLookAtStates>

- for developers: Change public interface of generation of random ints and
  doubles in the `RandomNumberGenerator` class.
  <https://issues.fast-downward.org/issue1026>

- for developers: Use `RandomNumberGenerator` class in `VariableOrderFinder`.
  <https://issues.fast-downward.org/issue1032>

- infrastructure: Add support for GitHub actions.
  <https://issues.fast-downward.org/issue940>

- infrastructure: Add CPLEX support to our GitHub Actions for Windows.
  <https://issues.fast-downward.org/issue1005>

- infrastructure: Decide on rules regarding software support and
  improve GitHub actions accordingly.
  <https://issues.fast-downward.org/issue1003>

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
