#ifndef MERGE_AND_SHRINK_FTS_FACTORY_H
#define MERGE_AND_SHRINK_FTS_FACTORY_H

/*
  Factory for factored transition systems.

  Takes a planning task and produces a factored transition system that
  represents the planning task. This provides the main bridge from
  planning tasks to the concepts on which merge-and-shrink abstractions
  are based (transition systems, labels, etc.). The "internal" classes of
  merge-and-shrink should not need to know about planning task concepts.
*/

class TaskProxy;

namespace utils {
class Timer;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
enum class Verbosity;

/*
  Note on how the time limit is handled: if running out of time during
  computing the initialization of transition system data (except transitions,
  which are built separately), then we store the index of the last transition
  system with valid data and only do the remaining computation for all
  valid transition systems. This certainly violates the time limit to some
  extent. When running out of time during the computation of the actual
  transitions, we immediately stop and only return all factors that have been
  completely constructed, including transitions.
*/
extern FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity,
    const double max_time,
    const utils::Timer &timer);
}

#endif
