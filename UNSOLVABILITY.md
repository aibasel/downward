# Reporting unsolvability

This document describes how Fast Downward detects and reports that a planning
task has **no solution**, what information the planner gives you in each case,
and what changed in the `unsolvability-reporting` branch.

Unsolvability can be discovered in two places:

1. **The translator** (`src/translate/`), during preprocessing, before any
   search runs.
2. **The search component** (`src/search/`), either while building a heuristic
   or while exploring the state space.

Both layers communicate the outcome through the planner's **exit code**.

## Exit codes

| Code | Name | Message | Meaning |
|------|------|---------|---------|
| 10 | `TRANSLATE_UNSOLVABLE` | *(translator prints a reason, see below)* | The translator proved unsolvability during preprocessing. |
| 11 | `SEARCH_UNSOLVABLE` | `Task is provably unsolvable.` | The search proved that no plan exists (within the current cost bound, if one is set). |
| 12 | `SEARCH_UNSOLVED_INCOMPLETE` | `Search stopped without finding a solution.` | No plan was found, but unsolvability could **not** be proven (the algorithm is incomplete, gave up, or pruned states unsoundly). |

The full list of exit codes lives in `driver/returncodes.py`.

> **What changed:** previously, exit codes 10 and 11 were almost never emitted
> for a task that "just" turned out to be unsolvable. The translator always
> wrote a trivially-unsolvable task and exited `0`; standard search (A\*, lazy
> search, …) reported `12` even after *exhaustively* proving unsolvability.
> Now the translator exits `10`, and an exhaustive search reports `11` whenever
> the proof is sound (see [Soundness](#soundness-when-is-exhaustion-a-proof)).

## Translator reasons (exit code 10)

When the translator detects unsolvability it prints a one-line reason, emits a
trivially-unsolvable SAS task (so the search component still behaves as before
if it is run on the file directly), and exits with `TRANSLATE_UNSOLVABLE` (10).
All four cases funnel through `unsolvable_sas_task()` in
`src/translate/main.py`.

| Reason | When it triggers | Reported detail |
|--------|------------------|-----------------|
| **No relaxed solution** | The goal is unreachable even in the delete relaxation (the strongest "easy" reachability check). | **Lists the positive goal atoms that are not relaxed reachable** (and are not already true in the initial state). |
| **Goal violates a mutex** | Two goal facts can never hold simultaneously according to the discovered mutex groups. | **Names the pair of mutually exclusive goal atoms.** |
| **Trivially false goal** | The instantiated goal collapses to "impossible" because of static facts (e.g. a negated static atom that always holds). See issue1055. | Clarifying message describing the static-false goal. |
| **Simplified to trivially false goal** | While filtering unreachable propositions, the goal becomes contradictory (`simplify.Impossible`). | Message identifying the simplification step. |

> **What changed:** all four reasons existed before, but they were printed as
> bare strings ("No relaxed solution", "Goal violates a mutex", …) and the
> translator exited `0`. The branch (a) propagates the dedicated exit code
> `10`, and (b) enriches the first two reasons with the concrete offending
> atoms. The detail is computed read-only and never affects the translation.

Example output:

```
No relaxed solution: the following goal atoms are not reachable even in the
delete relaxation: Atom q()! Generating unsolvable task...
```

```
Goal violates a mutex: the goal atoms Atom at(b) and Atom at(c) are mutually
exclusive! Generating unsolvable task...
```

## Search reasons

The search component can report unsolvability in two fundamentally different
ways.

### 1. Unsolvability proven while building a heuristic

Several heuristics analyse (an abstraction of) the task before search begins
and can already prove that no plan exists:

* **Pattern database CEGAR** (`pdbs/cegar.cc`) calls
  `exit_with(SEARCH_UNSOLVABLE)` directly when a pattern's projection is
  unsolvable.
* **Merge-and-shrink** (`merge_and_shrink/merge_and_shrink_heuristic.cc`)
  extracts an *unsolvable factor* and uses it as the heuristic; it then returns
  `DEAD_END` for the initial state.
* **Cartesian abstractions** (`cartesian_abstractions/cegar.cc`) report
  "Abstract task is unsolvable" and the resulting heuristic is infinite on the
  initial state.
* Any **admissible heuristic** (`lmcut`, `hmax`, PDBs, landmark heuristics,
  merge-and-shrink, Cartesian, …) that returns `DEAD_END` / infinity for the
  initial state effectively proves unsolvability, because for these heuristics
  dead ends are *reliable*.

In the heuristic-returns-dead-end cases, the search opens no states, exhausts
the (empty) open list immediately, and now reports `SEARCH_UNSOLVABLE` (11).

### 2. Unsolvability proven by exhausting the state space

`eager_search` and `lazy_search` log

```
Completely explored state space -- no solution!
```

when the open list runs empty. If the exploration was complete, this **is** a
proof of unsolvability. The search now returns the new `UNSOLVABLE` search
status (see `enum SearchStatus` in `src/search/search_algorithm.h`), which
`planner.cc` maps to exit code `11`.

### Enforced hill climbing

`ehc` is incomplete, so it generally reports `SEARCH_UNSOLVED_INCOMPLETE` (12)
when it gives up. The one exception (pre-existing) is when the **initial state
is a dead end** and the evaluator's dead ends are reliable, in which case it
already reported `SEARCH_UNSOLVABLE`.

## Soundness: when is exhaustion a proof?

Exhausting the open list only proves unsolvability if no reachable state was
discarded unsoundly. The relevant subtlety is **dead-end pruning**:

* Open lists distinguish `is_dead_end()` (used for pruning) from
  `is_reliable_dead_end()`. A dead end is *reliable* only if the evaluator
  guarantees the state truly cannot reach the goal.
* Relaxation-based heuristics report **unreliable** dead ends when the task has
  axioms or conditional effects (see `HMHeuristic::dead_ends_are_reliable`,
  `RelaxationHeuristic::dead_ends_are_reliable`, etc.). Pruning such a state may
  remove a genuine path to the goal, so an "empty" open list no longer proves
  unsolvability.

To stay sound, `eager_search` and `lazy_search` track a flag
`exhaustion_proves_unsolvability` (initialised to `true`). Whenever a state is
pruned for which `is_dead_end()` holds but `is_reliable_dead_end()` does not,
the flag is cleared. On exhaustion:

* `exhaustion_proves_unsolvability == true`  → return `UNSOLVABLE` → exit `11`.
* `exhaustion_proves_unsolvability == false` → return `FAILED`     → exit `12`.

Cost-bound pruning (`g + cost(op) >= bound`) does **not** invalidate the proof:
exit code `11` means "provably unsolvable with the given bound". Completeness-
preserving operator pruning (e.g. stubborn sets / partial-order reduction) is
also fine, because it preserves at least one path to every reachable goal.

This is why, for example, `astar(hm())` on a *solvable* task that contains
axioms still reports `12` (the unreliable dead ends make exhaustion
inconclusive) rather than wrongly claiming unsolvability.

## Quick reference: how to read the result

* **Exit 10** — the task is unsolvable for a structural reason found before
  search; read the translator line for *which* goal atoms / mutex are to blame.
* **Exit 11** — the search proved unsolvability (heuristic dead end on the
  initial state, or a complete exhaustive search). Trustworthy as a proof
  (modulo the cost bound).
* **Exit 12** — no plan found, but **not** a proof. The configuration is
  incomplete or relied on unreliable pruning; try a complete, admissible
  configuration (e.g. `astar(blind())`) to get a definitive `11`/solution.
