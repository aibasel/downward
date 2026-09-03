# Release notes

Fast Downward has been in development since 2003, but the current
timed release model was not adopted until 2019. This file documents
the changes since the first timed release, Fast Downward 19.06.

For more details, check the repository history
(<https://github.com/aibasel/downward>) and the issue tracker
(<https://issues.fast-downward.org>). Repository branches are named
after the corresponding tracker issues.

## Fast Downward 26.6

Released on September 10, 2026.

Highlights:

- The version number no longer uses leading zeros (26.6 instead of
  26.06). Our previous convention followed Ubuntu. Our new one follows
  PyPI, which strips leading zeros. This change is in preparation of a
  future release on PyPI.

- The translator is now available as the standalone package
  `fast-downward.translate` on PyPI. The version number matches the
  corresponding Fast Downward release. For example, install this
  version with `pip install fast-downward.translate==26.6`. You can
  also invoke the translator directly from source, circumventing the
  `fast-downward.py` script, by running `python3 -m
  fast_downward.translate` from directory `src/translate`. See
  issue1235 and issue1185 for details.

- The planner now has better knowledge of which of its configurations
  are complete and therefore returns the exit code `SEARCH_UNSOLVABLE`
  in more cases where it previously returned
  `SEARCH_UNSOLVED_INCOMPLETE`. There is also a new exit code
  `SEARCH_UNSOLVABLE_WITHIN_BOUND` for cases where the user passed a
  bound to the search algorithm. See issue1201 for details.

- PDDL error detection and handling are much improved. See issue220
  for details.

- The search component now performs more checks on the validity of its
  input file (`output.sas` by default). This is only relevant for
  users that used the search component directly on tasks that were not
  produced by Fast Downward's translator. See issue1146 for details.

- There is a new translator option
  `--condition-normalization-strategy` for different ways of handling
  complex condititions (preconditions, goals, effect conditions, axiom
  bodies). The old behaviour is the default. See issue1222 for
  details.

- There is a new translator option `--keep-no-ops` that can be used to
  preserve operators without effects, which are pruned by default. See
  issue1184 for details. As of this writing, there is a bug related to
  enabling this option, which can be worked around by also enabling
  `--keep-unimportant-variables`. See issue1241 for details.

- Evaluators (including heuristics) no longer have a `transform`
  argument to modify action costs. Instead, the evaluator
  `eval_modify_costs` can be used to invoke another evaluator with
  modified costs. Configurations such as `h(...,
  transform=adapt_costs(cost_type=X))` must be rewritten as
  `eval_modify_costs(h(...), cost_type=X)`. See issue559 for details.

- The landmark factories `lm_exhaust` and `lm_rhw` no longer have the
  option `only_causal_landmarks`. For `lm_exhaust`, there is a new
  option `use_unary_relaxation`, which has a similar effect to the
  removed option, but a cleaner and more efficient implementation. See
  issue1074 for details.

- The `lmcut` heuristic has two new options to control the cut
  generation. The default setting
  `lmcut(goal_zone_detection=true,border_detection=true)` often leads
  to an improved heuristic compared to the old behaviour. This
  implements an idea by Lauer and Fickert (HSDIP workshop at ICAPS
  2020). See issue1228 for details.

- We fixed a bug in the `merge_and_shrink` evaluator, which caused the
  wrong option values to be used in many cases. See issue1173 for
  details.

- For developers: the translator is now a Python package
  `fast_downward.translate`. The code now is
  in the directory `src/translate/fast_downward/translate` (previously
  `src/translate`). See issue1235 and issue1185 for details.

- For developers: we have solved (although as a work-in-progress) the
  "component interaction problem", which has been a major open issue
  for more than 11 years. Components such as heuristics and search
  algorithms now have two forms: a task-independent form and a
  task-specific form. See issue559 for details. You can read more
  about the design in the blog:
  <https://www.fast-downward.org/latest/for-developers/blog/component-interaction/>

Details:

- [issue1235] Make translator available on PyPI.

    Added the necessary scripts and infrastructure to upload the
    translator as a standalone package to PyPI.

    The plan is to make a translator release every time we make a regular
    Fast Downward release. The process is documented in our release docs.

    The translator used to be a module called `translate`, and the code
    lived in `src/translate`.

    It is now a module called `fast_downward/translate`, and the code
    lives in `src/translate/fast_downward/translate`.

    We changed the module name to support uploading the translator as a
    stand-alone package on PyPI with a reasonable name, and for the
    overall directory structure we also considered future plans with other
    Python code living alongside the translator. See the issue for more
    discussion.

    Inside the translator code, while the diff looks huge, the only real
    changes are replacing all occurrences of `from translate import XYZ`
    with `from fast_downward.translate import XYZ`.

- [issue1231] add early stopping to iterated search. (#302)

    Instead of having the user pass a `continue_on_fail` flag for the
    iterated search, we use the completeness checks to detect whether or not
    continuing the search makes sense, otherwise we stop early.

    Existing configs that call the iterated search with
    `continue_on_fail=<whatever>` should remove this part as the keyword
    argument `continue_on_fail` is no longer known to the iterated search.

- [issue1224] Switch two-sided to one-sided constraints in the LP interface.

    Constraints in the LP interface are now one-sided constraints, i.e. expr <= RHS,
    instead of two-sided constraints, i.e. LB <= expr <= UB. This generally improves
    the performance of CPLEX configurations and has a negative impact on the Rankooh
    and Rintanen implementation of delete-relaxation constraints for SoPlex.

    Developers: you should represent two-sided constraints (LB <= expr <= UB) into a
    pair of one-sided constraints (expr <= UB, expr >= LB).

- [issue1229] Isolate domain transition graph in RHW landmark factory.

    This is a pure refactoring without noteworthy impact on planner performance.

- [issue1222] Add translator options to decompose complex conditions.

    Complex conditions can be handled in the following three different ways using the new translator option `--condition-normalization-strategy`.

    - `dnf` (default): Replace complex goals with a derived predicate defined by a single axiom and naively convert all conditions to DNF.
    - `axiomatize_disjunctions`: Replace complex goals with a derived predicate defined by a single axiom and decompose all disjunctions using derived predicates.
    - `axiomatize_disjunctions_existentials`: Decompose every disjunction and existential quantifier in action conditions and the goal, while decomposing only disjunctions in axiom bodies.

    The motivation for the three options are the following.

    - Option `dnf` tries to avoid axioms, as they are not supported by many planner components.
    - Option `axiomatize_disjunctions` decomposes complex conditions and thus avoids the exponential behavior of option `dnf`. However, it may introduce (more) axioms.
    - Option `axiomatize_disjunctions_existentials` generally introduces the largest number of axioms, but it does not produce multiple operator instances for the same original operator (as the other two options may do). This can be particularly helpful for reasoning about the plan space, for example in plan counting and enumeration.

- [issue1228] alternative precondition choice functions for LM-Cut

    We added two options (goal_zone_detection and border_detection) to the
    LM-cut computation, following Lauer and Fickert (2020).

    The tie-breaking strategies control how the h^max supporter of a zero-cost
    operator is selected while creating the goal zone.

    We also made some minor code-style adjustments to conform to the coding
    conventions.

- [issue1201] Handle new exit code in driver.

    issue1201 added the new exit code `UNSOLVABLE_WITHIN_BOUND` that
    requires separate handling when running a portfolio.

- [issue1232] Rename translator problem file input from "task" to "problem".

    We aligned the terminology in the translator code base with the
    documentation on the website, making it more consistent.

    A "task" refers to the full task with initial state, actions, axioms,
    goal...  For the part corresponding to the PDDL problem file (objects,
    initial state, goal), we use the term "problem".

    We changed the name of the translator input and internal names
    accordingly.

- [issue1201] Check whether a configuration guarantees completeness.

    We added a check determining whether the chosen configuration guarantees completeness (within the given bound) of the search. We return the exit code `SEARCH_UNSOLVABLE` in more situations where we can now guarantee completeness of the search and we added the new exit code `SEARCH_UNSOLVABLE_WITHIN_BOUND` that is returned when completeness is guaranteed only within the given bound. We also added tests for both exit codes `SEARCH_UNSOLVABLE` and `SEARCH_UNSOLVABLE_WITHIN_BOUND`.

- [issue1223] Document translator options on the Fast Downward website.

    We now have an (auto-generated) page with the translator options
    on the Fast Downward website (e.g.
    <https://www.fast-downward.org/latest/documentation/translator-options/>).
    We also use the terms "task" and "problem" in the documentation in a
    more consistent fashion.

- [issue559] Switch to Task-Independent Components.

    We redesigned how components specified on the command line are associated with
    a specific task. Most of the changes are internal, but one user-visible change
    is that heuristics no longer accept an optional task transformation via the
    `transform` argument. The only supported task transformation was to evaluate
    the heuristic under a modified cost function. This functionality is now
    provided by the `eval_modify_costs` evaluator. Configurations such as
      `h(..., transform=adapt_costs(cost_type=X))`
    must be rewritten as
      `eval_modify_costs(h(...), cost_type=X)`.

    Developers: Internally, components such as heuristics and search algorithms now
    have two forms: a task-independent form and a task-specific form.
    Task-independent components can often use the templated helper
    `components::make_auto_task_independent_component`. It stores constructor
    arguments until a task is bound, recursively binds any subcomponents to that
    task, and then constructs the task-specific component from the resulting
    arguments.

    Arguments for nested components should now take task-independent types. For
    example, the sum evaluator now takes a list of `TaskIndependentEvaluator`s
    instead of a list of `Evaluator`s.

    You can read more about the design in the blog:
    <https://www.fast-downward.org/latest/for-developers/blog/component-interaction/>

- [issue1205] restrict weight parameter for weighted evaluator to be non-negative. (#294)

- [issue1214] prevent synopsis overwriting. (#289)

    Move part that would cause the overwriting of the synopsis to a note.

    Add assertions to prevent overwriting in the future.

- [issue1200] Add reference to documentation of AbstractTask.  (#282)

- [issue1217] translator: separate argument parser construction from parsing

    We now have two separate functions for creating the argument parser of the
    translator and parsing the arguments using this parser. This used to be
    combined in one function.

    For Fast Downward itself, this change makes no difference, but it makes it
    easier for other users of the translator to hook into the argument parsing
    process.

- [issue1193] Update clang-tidy-16 to clang-tidy-18. (#279)

- [issue1198] Refactor responsibilities of RawRegistry, TypeRegistry and Registry.

    Refactor the construction of the feature registry and type registry, so
    repeated construction cannot lead to errors and functionality is moved
    out of the RawRegistry class.

- [issue1197] Handle unstratified axioms error. (#280)

- [issue1195] Remove (buggy and unused) computation of CG heuristic cache statistics.

- [issue1063] translator: Compute indirect subtypes more efficiently.

- [issue1077] Remove the pattern collection generator "combo()".

    This pattern collection generator was ad hoc, lacked a description
    in the documentation and was never mentioned in the literature to
    the best of our knowledge.

    We recommend using the following generators instead:
    - with cpdbs: multiple_cegar or hillclimbing
    - with zopdbs: disjoint_cegar

    See the issue tracker for more discussion.

- [issue1191] Refactor interface of MergeSelector.

    MergeSelector now has two variants of the method for choosing the best
    merge candidate for a given factored transition system: one that
    considers all merge candidates for the given factored transition
    system, and another one that only considers the given (sub)set of merge
    candidates.

- [issue1192] Fix default value handling in MIASM scoring function.

    The MIASM scoring function now handles default/missing values (-1)
    for parameters max_states, max_states_before_merge, and
    shrink_threshold_before_merge in the same way as the merge-and-shrink
    heuristic does.

- [issue1190] Update documentation on the Fast Downward website.

    The online documentation for Fast Downward has been updated.
    Especially the steps for getting started with Fast Downward and running
    the planner should be clearer now.

- [issue1185] Make the translator a python package.

    The translator is now a python package (within Fast Downward).
    This does not affect how Fast Downward is called, but if you want to
    call the translator directly then use 'python3 -m translate' instead of
    'python3 translate/main.py' (which replaced the previous 'translate.py').

- [issue1146] Validate SAS file (#227)

    The search component now does more checking of the validity of its input file.
    We add a more thorough diagnostic message when invalid input is provided. We
    restructured to explicitly split the parser, lexer and internal task
    representation. The SAS task parsing now takes roughly 25% more time in our
    experiments.

- [issue1189] Check code style with clang-format instead of uncrustify.

    Because uncrustify does currently not correctly handle modern C++ features such as concepts, we switch to the more up-to-date clang-format. This makes our style checks more opinionated and more strict. Our configuration is based on the LLVM style and defined in `.clang-format`. To reformat all source files according to the configuration you can for example run the following command in the repository root: `clang-format -i src/search/**/*.{cc,h}`.

- [issue1186] Refactor the EagerSearch step function without changing the search behavior.

    - Split logic from step() into get_next_node_to_expand() and expand().
    - Move successor generation to a dedicated generate_successors() method.
    - Add helper functions for collecting preferred operators.

- [issue1188] Add a script that creates a local build of the website.

    The script is located in misc/website and combines the generated
    documentation, the documentation from docs/ and the one from
    the separate downward-markdown repository.

- [issue1184] translator: Add --keep-no-ops option

    We added a new option `--keep-no-ops` to the translator, which defaults
    to false.

    To enable the option, add `--translate-options --keep-no-ops` to the end
    of the argument list for `fast-downward.py`.

    The translator component usually prunes all ground actions that have no
    effects. When the new option is set to true, these actions are no longer
    pruned. There is generally no good reason to use this option in regular
    classical planning settings. It is primarily intended for users that extend
    Fast Downward to richer planning formalisms such as FOND planning or
    other scenarios where the planner is part of a larger pipeline.

    On the extended downward-benchmark benchmark set, 1208 out of 3955
    tasks for which we could finish translation within the resource limits
    in our experiments produce at least one action with no effect with the
    new flag.

- [issue 1163] Print code that was used to build the code

    We now print the code revision that was used to build the search code.
    This is printed to to the log at the start of every planner run and
    also be queried directly with `./fast-downward.py --version`.
    As part of this change, we dropped support for builds outside of the
    `builds` directory.

- [issue992] Make landmark code more readable.

    Refactor all files in `src/search/landmarks` with the main objective to
    structure them to clarify what they do. The main changes include
     - breaking apart long functions,
     - renaming variables and functions, and
     - making code consistent with our coding conventions.
    Further changes include
     - using more suitable data structures,
     - improving simple procedures, and
     - using the `ranges` library.

    The general functionality remains unchanged, or at least that was our
    intention. We verified experimentally that this appears to be the case
    for the most commonly used landmark configurations. Performance was
    affected positively over all, mainly because landmark generation is
    faster with most factories. Most remarkably, the h^m landmark factory
    solves 24 more problems, but also LAMA improves slightly in terms of
    coverage (3-4 more problems solved).

- [issue1183] Improve how RHW determines landmark preconditions.

    The approximation of landmark preconditions may now yield larger, more accurate
    sets in the presence of conditional effects. This potentially leads to more
    landmarks found in the RHW backchaining landmark generation. The change has no
    effect in the optimal benchmark suite and only affects four domains in the
    satisficing benchmark suite: flashfill (~69% more landmarks), miconic-fulladl
    (~126% more landmarks), miconic-simpleadl (~88% more landmarks), and settlers
    (~88% more landmarks). This increase has no significant impact on the search
    behavior of LAMA and LAMA-first.

- [issue220] Improve PDDL error detection and handling

    - Conditions are allowed to be empty.
    - We print a warning if a predicate is defined in the predicates-section that
      uses the same name as a type.
    - Checking if the arity is correct now also happens for problem files and not
      only for domain files.
    - "number" cannot be defined as a (new) type in the types section because it is
      reserved as a special type.
    - The keyword "either" is allowed only in the predicates section because the
      storage domain uses it. It is not supported though.
    - We warn if duplicate action names occur.
    - The end of a domain file is now better handled. Before, everything except
      "metric" coming after the goal was ignored. Now nothing except the metric
      section is allowed after the goal.
    - An empty list before a type declaration raises a warning (not throws an error
      because the woodworking domain contains some files where this would crash).
    - Existence checks for predicate names and terms.

- [issue1182] Use std::make_unique instead of utils::make_unique_ptr.

- [issue1174] Retire autodoc for the wiki

    We used autodoc to generate the documentation of the search plugins
    for the wiki. Since the wiki has been retired, we remove this functionality
    from the code base. The tox test now instead tests the generation of the
    downward website (including the generation of the search plugin
    documentation).

- [issue1174] Polish markdown documentation

    We improved the markdown documentation while checking that everything has been
    carried over from the old wiki.  This includes also some updates of outdated
    examples (e.g.  replacing references to the lmcount heuristic with the
    corresponding new heuristics and replacing  the old --evaluator syntax with
    let(...).

- [issue1179] Avoid hardcoding bugfix releases in vagrantfile.

    In the Vagrantfile, the latest bugfix release is now cloned from the corresponding branch instead of using a hardcoded tag.

- [issue1174] Move version-dependent documentation into the main repository.

    In an effort to retire the old wiki, we move the parts of the documentation that
    belong to a specific code version into the main code repository. The documentation
    of the search plugin types is now also generated in markdown (from Scorpion).

- [issue1074] Change behavior of option to compute only causal landmarks.

    We remove the option to consider only causal landmarks (`only_causal_landmarks`)
    from the landmark factories `lm_exhaust` and `lm_rhw`. For `lm_exhaust` we add a
    new boolean option `use_unary_relaxation` which yields an equivalent landmark
    graph as a result. Its effect in the landmark computation is different, though,
    as it no longer constitutes a post-processing to remove all non-causal
    landmarks, but makes sure only causal landmarks are added in the first place.
    This results in lower landmark generation time for `lm_exhaust` with this option
    active; on average, landmark generation takes 25-30% of the time it used to.

    Removing the option from `lm_rhw` affects the portfolios seq-sat-fdss-2018 and
    seq-sat-fdss-2023, which used to have some configurations with the flag on and
    others with the flag off. Simply removing the option (essentially turning the
    flag off in all configurations) didn't have a negative effect as can be seen
    here:
    <https://ai.dmi.unibas.ch/_experiments/ai/downward/issue1074/data/issue1074-v5-sat-eval/issue1074-v5-portfolios-issue1074-base-issue1074-v5-compare.html>

- [issue1081] Move error handling into constructors.

    Parameters are no longer checked in the Feature of a component but in its constructor.

- [issue1175] Fix Vagrantfile template after vagrant API change.

- [issue1173] Fix parameter order for MergeAndShrinkAlgorithm.

    We passed the options for the merge_and_shrink evaluator into
    the constructor of an internal class in the wrong order, leading to
    wrong behaviour. For example, the boolean option prune_unreachable_states
    was interpreted as max_states, so that max_states was set to 0
    or 1 based on the boolean flag. This is now fixed.

- [issue1170] SCC merge strategy: remove option to use a merge tree.

    Conceptually, it doesn't make too much sense to use a precomputed merge strategy for a subset of factors, and the option was never used in a paper. We remove it to simplify the code and facilitate the integration of other options.

- [issue1165] Stop getting landmarks by atom outside of landmark factories.

    We no longer assume that landmarks do not overlap outside of the landmark
    factories. Semantics of the code do not change and runtimes stay similar.

## Fast Downward 24.06

Released on October 10, 2024.

Highlights:

- We have improved the translator in several dimensions. Key
  improvements include better error reporting for invalid PDDL,
  revised and faster version of the invariant algorithm and full
  support for negated predicates in the goal.

- Negated axioms are now computed in the search component, and only in
  configurations that actually use them. This eliminates a worst-case
  exponential performance bottleneck in cases where negated axioms are
  not used. Heuristics that use negated axioms have a new option
  (`axioms=approximate_negative`) to avoid this computation at the
  cost of some heuristic accuracy.

- There are many improvements to the `landmark_sum` and
  `landmark_cost_partitioning` heuristics. This includes a new theory
  of landmark progression that fixes a gap in the completeness in
  search configurations like LAMA and can deal directly with cyclic
  landmark orders, support for reasonable orders in admissible
  landmark heuristics, a cleaner definition of preferred operators,
  deterministic synthesis of reasonable orders, and performance
  improvements. Please note that the command-line options for landmark
  heuristics have changed.

- We added a new set of operator-counting constraints
  (`delete_relaxation_rr_constraints`), which implements the
  delete-relaxation operator-counting constraints by Rankooh and
  Rintanen. The old `delete_relaxation_constraints` plugin using the
  constraints by Imai and Fukunaga has been renamed to
  `delete_relaxation_if_constraints`.

- We added the alias `seq-sat-fdss-2023` for the satisficing Fast
  Downward Stone Soup 2023 portfolio from IPC 2023.

- For developers: planner component objects (such as heuristics or
  search algorithms) are now constructed with individual parameters
  rather than having all parameters encapsulated in an `Options`
  object. (We mention this primarily for developers maintaining their
  own forks, as this change affects more than 200 source files.)

Details:

- bug fix: The planner now invokes destructors when terminating
  (except for emergency situations where this is not possible) and
  exits gracefully when running out of memory.
  <https://issues.fast-downward.org/issue1138>
  <https://issues.fast-downward.org/issue984>

- bug fix: Removed a dangling pointer in the state registry caused by
  state copying.
  <https://issues.fast-downward.org/issue1115>

- build: On M2 Macs it is now easier to build Fast Downward with CPLEX.

- build: We now support static linking of CPLEX.
  <https://issues.fast-downward.org/issue1122>

- build: We modernized the CMake build system by updating the
  requirement to CMake 3.16. New files must now be listed in
  src/search/CMakeLists.txt for compilation. For Soplex support, users
  need to set the soplex_DIR environment variable instead of
  DOWNWARD_SOPLEX_ROOT. Furthermore, we renamed CMake options from
  PLUGIN_FF_HEURISTIC_ENABLED to LIBRARY_FF_HEURISTIC_ENABLED.
  <https://issues.fast-downward.org/issue1097>

- build: We added support for the `Validate` binary from newer VAL
  versions, defaulting to `Validate` if `validate` is not found on the
  PATH.
  <https://issues.fast-downward.org/issue1123>

- build, for developers: We introduced the
  `-Wzero-as-null-pointer-constant` flag to warn when 0 is used
  instead of `nullptr` and the `-Wmissing-declarations` flag to detect
  global functions not declared in header files, promoting static
  declarations for visibility control.
  <https://issues.fast-downward.org/issue1112>
  <https://issues.fast-downward.org/issue1107>

- command line, for users: usage errors are now printed to stderr.
  <https://issues.fast-downward.org/issue1084>

- command line, bug fix: Line breaks in command lines are now
  correctly handled on Windows.
  <https://issues.fast-downward.org/issue1120>

- driver: When running a portfolio, its component now prints the
  absolute runtime as well as the relative runtime.
  <https://issues.fast-downward.org/issue1131>

- driver: We have added the alias `seq-sat-fdss-2023` for the
  satisficing Fast Downward Stone Soup 2023 portfolio.
  <https://issues.fast-downward.org/issue1110>

- infrastructure: We have restructured the documentation for building
  and running experiments. The build instructions are in the
  [BUILD.md](BUILD.md) file. Developer-specific build information
  has been moved to the "for developers" wiki section.
  <https://issues.fast-downward.org/issue961>.

- infrastructure: We have removed the experiments directory from the
  repository.
  <https://issues.fast-downward.org/issue1116>

- infrastructure: We have updated the operating system versions and
  software versions used for our GitHub actions test suite.
  <https://issues.fast-downward.org/issue1142>

- infrastructure: Windows builds using CPLEX in GitHub actions now
  work with arbitrary repository names.
  <https://issues.fast-downward.org/issue1121>

- landmarks, for users: We no longer break cycles in
  landmark graphs because landmark progression can now deal
  with cycles. Obedient-reasonable orderings are no longer used
  because they had little impact on performance in our experiments
  and they are not well-supported by theory.
  <https://issues.fast-downward.org/issue996>
  <https://issues.fast-downward.org/issue1089>

- landmarks, for users: Configurations with landmark heuristics are up
  to 30% faster due to data structure optimizations.
  <https://issues.fast-downward.org/issue1126>

- landmarks, for users: The landmark cost partitioning heuristic now
  uses an enumeration to define the type of cost partitioning instead
  of a Boolean. This affects the command line: `optimal={true,false}`
  is now `cost_partitioning={optimal,uniform}`.
  <https://issues.fast-downward.org/issue1091>

- landmarks, for developers: We updated variable, function, class and
  file names within the landmark cost partitioning code.
  <https://issues.fast-downward.org/issue1108>

- landmarks, for users: The algorithm for generating reasonable
  landmark orderings is now deterministic and finds more orderings.
  This has a positive impact on performance (number of
  expansions, plan quality for satisficing configurations).
  <https://issues.fast-downward.org/issue383>

- landmarks, for users: the landmark heuristics now consider an
  operator preferred iff it achieves a landmark that is needed in the
  future according to the heuristic. This replaces the previous, much
  more convoluted definition.
  <https://issues.fast-downward.org/issue1075>

- landmarks, bug fix: Landmark progression is now sound. The new
  progression stores more information per state, leading to higher
  memory requirements, but overall performance is only minimally
  affected. With this change, it is now safe to use reasonable
  orderings in the `landmark_cost_partitioning` heuristic. Since this
  is in general beneficial, The `seq-opt-bjolp` alias now uses
  reasonable orderings.
  <https://issues.fast-downward.org/issue1036>

- landmarks, bug fix: We no longer wrongly assert that conjunctive
  landmarks do not overlap with simple or disjunctive landmarks.
  <https://issues.fast-downward.org/issue1087>

- LP/MIP solvers, bug fix: Empty constraint systems no longer lead to
  crashes when using CPLEX.
  <https://issues.fast-downward.org/issue1111>

- LP/MIP solvers, bug fix: We now call the correct methods of the LP
  solvers for setting variable bounds.
  <https://issues.fast-downward.org/issue1119>
  <https://issues.fast-downward.org/issue1118>

- merge-and-shrink, for developers: We refactored and simplified the code
  of the `merge_sccs` merge strategy.
  <https://issues.fast-downward.org/issue1105>

- operator counting, for users: We added
  `delete_relaxation_rr_constraints`, which implements the
  delete-relaxation operator-counting constraints described in
  "Efficient Computation and Informative Estimation of h<sup>+</sup>
  by Integer and Linear Programming" (Rankooh and Rintanen, ICAPS
  2022). The old `delete_relaxation_constraints` plugin is now called
  `delete_relaxation_if_constraints`.
  For details, see
  <https://www.fast-downward.org/Doc/ConstraintGenerator#Delete_relaxation_constraints_from_Rankooh_and_Rintanen>
  <https://issues.fast-downward.org/issue1134>

- option parser, for users and developers: constructors no longer use
  an encapsulated `Options` object, but take their parameters
  directly. As a side effect, some command-line options now take their
  parameters in a different order.
  <https://issues.fast-downward.org/issue1082>

- option parser, for developers: We now support string arguments in
  double quotes. Strings may use escape symbols `\"`, `\\`, and `\n`
  for double quotes, backslashes and newlines.
  <https://issues.fast-downward.org/issue1106>

- potential heuristics, bug fix: The potential optimizer now supports
  effects that set a variable to a value that is already required by a
  precondition. (The code will never generate such effects, but this
  makes it possible to use task transformations that do generate such
  effects.)
  <https://issues.fast-downward.org/issue1150>

- search algorithms, bug fix: in the `eager` search algorithm, the
  setting `reopen_closed=false` also affected open nodes, not just
  closed nodes. This has now been fixed. Note that the previous
  behavior did not affect the optimality of A* because it does not use
  this setting.
  <https://issues.fast-downward.org/issue1140>

- search algorithms, bug fix: Correctly propagate plan cost bounds to
  components in iterated search in cases where manual cost bounds are
  combined with bounds derived from incumbent solutions.
  <https://issues.fast-downward.org/issue1130>

- translator and heuristics, for users: Negated axioms are now
  computed in the search component and only for heuristics that need
  them (relaxation heuristics, landmark heuristics, `cea` and `cg`).
  This can lead to a large performance improvement for configurations
  that do not use the aforementioned heuristics. We added a new option
  for heuristics using negated axioms:

  `axioms={approximate_negative,approximate_negative_cycles}`

  `approximate_negative_cycles` is the old behavior and the new
  default, while `approximate_negative` may result in less informative
  heuristics, but avoids a potentially exponential number of negated
  axioms.
  <https://issues.fast-downward.org/issue454>

- translator, for users: We added full support for negative literals
  in goals.
  <https://issues.fast-downward.org/issue1127>

- translator: We removed a source of nondeterminism in the translator.
  The translator should now be deterministic except for the effect of
  the invariant synthesis timeout (option
  `--invariant-generation-max-time`).
  <https://issues.fast-downward.org/issue879>

- translator, for users: We improved error reporting for invalid PDDL
  input.
  <https://issues.fast-downward.org/issue1030>

- translator, bug fix: Uninitialized numeric expressions are now
  handled correctly by the translator.
  <https://issues.fast-downward.org/issue913>

- translator, bug fix: There was a conceptual gap in the invariant
  synthesis algorithm. This has been fixed by a revised algorithm,
  which is also faster.
  <https://issues.fast-downward.org/issue1133>

## Fast Downward 23.06

Released on July 31, 2023.

Highlights:

- The option parser of the search component has been completely
  reimplemented. The new option parser has full error reporting where
  the old code crashed on errors or silently accepted them. This is
  also an important stepping stone towards a future use of Fast
  Downward as a library. With the change, the `--evaluator`,
  `--heuristic` and `--landmarks` options are now deprecated in favor
  of a new `let` syntax. For example,

    `--evaluator h=EVALUATOR --search SEARCH_ALGORITHM`

  is deprecated in favor of the expression

    `--search let(h, EVALUATOR, SEARCH_ALGORITHM)`

- We now compile using the C++20 standard, so all modern C++ features
  can be used as long as they are supported by all main compilers Fast
  Downward supports (see `README.md`).

- The linear programming and mixed integer programming features of
  Fast Downward now communicate directly with the LP/MIP solvers
  (SoPlex or CPLEX) rather than using the open solver interface OSI as
  an intermediary. This has also allowed us to move to more modern
  versions of these solvers.

- The `lmcount` heuristic has been split into two separate heuristics
  called `landmark_sum` and `landmark_cost_partitioning`. The former
  corresponds to the old `lmcount` with the option `admissible=false`,
  the latter to the old `lmcount` with the option `admissible=true`.

- The two new landmark heuristics (`landmark_sum` and
  `landmark_cost_partitioning`) compute preferred operators more
  efficiently than before. On average, this improves performance of
  LAMA-style planner configurations.

- The merge-and-shrink heuristic now stores labels and their
  transitions more efficiently, resulting in improved speed for
  merge-and-shrink heuristic construction.

- The MIASM merge strategy for merge-and-shrink (more precisely, the
  `sf_miasm` merge scoring function) now has an option to cache
  scores. Caching is enabled by default and greatly speeds up
  construction of MIASM-based merge-and-shrink heuristics.

Details:

- option parser: We implemented a new way of defining features and
  parsing them from the command line. The new parser now supports
  defining variables for features (heuristics and landmark graphs so
  far) as an expression within the option string. For example,
    `let(h, lmcut(), astar(h))`
  instantiates the `lmcut` heuristic, binds it to the variable `h`
  and uses it within the `astar` search algorithm.

  This change to the parser is an important stepping stone towards
  solving a more general problem about how components interact.
  Details of the new parser are described in a blog article. We
  also improved the documentation of enum values.
  <https://www.fast-downward.org/ForDevelopers/Blog/TheNewOptionParser>
  <https://issues.fast-downward.org/issue1073>
  <https://issues.fast-downward.org/issue1040>

- translator, for developers: Unreachable atoms are now filtered from
  mutex groups in an earlier processing step than before. In a few
  domains this leads to a different finite-domain representation. We
  could remove an old hack related to static literals in goals, which
  is no longer necessary. We also added some type annotations, in
  particular to the core data structures.
  <https://issues.fast-downward.org/issue1079>

- translator, for users: The change described in the previous entry
  can lead to Fast Downward producing a slightly different
  finite-domain representation in some cases.

- search algorithms, for developers: Instead of "search engine" we now
  say "search algorithm" everywhere. Among other things, this affects
  source file and directory names, namespaces, class names. The change
  is also reflected in the documentation.
  <https://issues.fast-downward.org/issue1099>

- Cartesian abstractions, for developers: The `cegar` directory and
  namespace have been renamed to `cartesian_abstractions` to avoid
  confusion with CEGAR-based code in the `pdbs` directory.
  <https://issues.fast-downward.org/issue1098>

- landmarks: We replaced the `lmcount` heuristic with two new
  heuristics called `landmark_sum` and `landmark_cost_partitioning`.
  No functionality is added or removed: `landmark_sum` replaces
  `lmcount` with the option `admissible=false` and
  `landmark_cost_partitioning` replaces `lmcount` with the option
  `admissible=true`.
  <https://issues.fast-downward.org/issue1071>

- landmarks: Refactor the computation of preferred operators in the
  landmark heuristics (`landmark_sum` and
  `landmark_cost_partitioning`). The change affects configurations
  based on LAMA that use preferred operators. While the semantics of
  the code did not change, the new version is slightly faster and can
  solve more tasks and/or improves plan quality in an anytime
  configuration within the same time limit.
  <https://issues.fast-downward.org/issue1070>

- merge-and-shrink: Improve the way that labels and their transitions
  are stored. We removed the class `LabelEquivalenceRelation`. Instead
  `TransitionSystem` now handles locally equivalent labels itself
  using a new `LocalLabelInfo` class and stores a mapping from global
  to local labels. Labels are now removed from label groups in batches
  during label reduction. With this change, we can now construct the
  atomic factored transition system (and the overall heuristic) in
  significantly more cases, reducing both memory usage and
  construction time.
  <https://issues.fast-downward.org/issue927>

- merge-and-shrink: The MIASM scoring function (feature `sf_miasm`)
  now has an option to cache scores for merge candidates, enabled by
  default. This greatly decreases computation time of M&S abstractions
  with this scoring function.
  <https://issues.fast-downward.org/issue1092>

- pattern databases: The pattern generators `cegar_pattern` and
  `random_pattern` now correctly support fractional values for the
  `max_time` parameter. Previously the fractional part was silently
  ignored. For example a timeout of 7.5 seconds was treated as 7
  seconds.

- pattern databases, for developers: We split off the computation of
  pattern databases from the `PatternDatabase` class.
  `PatternDatabase` now stores pattern, `hash_multipliers` and
  `num_states` in a new class `Projection`, used for ranking and
  unranking. PDBs can be computed via a function which uses an
  internal class `PatternDatabaseFactory`. Abstract operators live in
  their own files now, similar to `MatchTree`. Performance does not
  change due to this issue.
  <https://issues.fast-downward.org/issue1019>

- LP/MIP solvers: So far, we used the open solver interface OSI to
  communicate with MIP and LP solvers. We now replaced this with our
  own interface to have more direct control and fewer external
  dependencies. We now support CPLEX 22.1.1 and SoPlex from their
  current main branch (released versions <= 6.0.3 are incompatible
  with C++20). We removed the solver options for CLP and Gurobi in the
  process: CLP has much worse performance than SoPlex, which is also
  open source, while Gurobi was never practically possible to compile
  with, as we did not have a CMake find script for it. Performance
  with CPLEX increases in MIP configurations and stays roughly the
  same in LP configurations. Performance with SoPlex increases for
  potential heuristics and decreased slightly in other configurations.
  Our Windows builds forced static linking because of OSI. With OSI
  removed, they no longer do.
  <https://issues.fast-downward.org/issue1076>
  <https://issues.fast-downward.org/issue1093>
  <https://issues.fast-downward.org/issue1095>

- LP/MIP solvers, for developers: Previously, LP solvers could crash
  if a constraint included multiple terms for the same variable. We
  now protect against this with an assertion in debug mode. The change
  has no effect on release builds.
  <https://issues.fast-downward.org/issue1088>

- build: The build script now performs the requested builds in the
  given order. For example,
    `./build.py debug release`
  will perform a debug build and then a release build. Previously, the
  order was arbitrary, depending on Python dictionary order.
  <https://issues.fast-downward.org/issue1086>

- C++ standard: We now compile the planner using the C++20 standard.
  We added tests for GCC 12 and clang 14 but dropped tests for
  GCC 9, clang 10, and clang 11.

  The minimal supported compilers are now:
  * g++-10
  * clang-12
  * MSVC 19.34
  * AppleClang 13.0

  Language features supported by all of these compilers may now be
  used in the planner. For a list of such features and more details,
  see the issue tracker. The list includes `std::optional`, so we
  removed the implementation of `optional` in `src/search/ext`.
  <https://issues.fast-downward.org/issue1028>
  <https://issues.fast-downward.org/issue1094>

- Windows build: We now enable more compiler warnings on Windows
  builds. In detail, we no longer ignore warnings C4800, C4512, C4706,
  C4100, C4127, C4309, C4702, C4239, C4996, and C4505 on Windows
  builds as they no longer occur in our code. Warnings in GitHub
  actions are now also treated as errors in Windows builds.
  <https://issues.fast-downward.org/issue565>

- infrastructure: We fixed a problem with using CPLEX in the GitHub
  action builds on Ubuntu.
  <https://issues.fast-downward.org/issue1093>

- infrastructure: We update the version of uncrustify used to fix code
  formatting. Instructions on how to install the new version can be
  found on the wiki:
  <https://www.fast-downward.org/ForDevelopers/Uncrustify>.
  <https://issues.fast-downward.org/issue1090>

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
