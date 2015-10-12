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

#include <memory>

class FactoredTransitionSystem;
class Labels;
class TaskProxy;

// TODO: The labels should eventually be part of the FTS rather
// than something created externally.
extern FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy, std::shared_ptr<Labels> labels);

#endif
