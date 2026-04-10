#ifndef CARTESIAN_ABSTRACTIONS_SUBTASK_GENERATORS_H
#define CARTESIAN_ABSTRACTIONS_SUBTASK_GENERATORS_H

#include "../component.h"

#include <memory>
#include <vector>

class AbstractTask;
struct FactPair;

namespace landmarks {
class LandmarkNode;
}

namespace plugins {
class Options;
}

namespace utils {
class RandomNumberGenerator;
class LogProxy;
}

namespace cartesian_abstractions {
using Facts = std::vector<FactPair>;
using SharedTasks = std::vector<std::shared_ptr<AbstractTask>>;

enum class FactOrder {
    ORIGINAL,
    RANDOM,
    HADD_UP,
    HADD_DOWN
};

/*
  Create focused subtasks.
*/
class SubtaskGenerator : public components::TaskSpecificComponent {
public:
    explicit SubtaskGenerator(const std::shared_ptr<AbstractTask> &task);
    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task,
        utils::LogProxy &log) const = 0;
};

using TaskIndependentSubtaskGenerator =
    components::TaskIndependentComponent<SubtaskGenerator>;

/*
  Return copies of the original task.
*/
class TaskDuplicator : public SubtaskGenerator {
    int num_copies;

public:
    TaskDuplicator(const std::shared_ptr<AbstractTask> &task, int copies);

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task,
        utils::LogProxy &log) const override;
};

/*
  Use ModifiedGoalsTask to return a subtask for each goal fact.
*/
class GoalDecomposition : public SubtaskGenerator {
    FactOrder fact_order;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

public:
    GoalDecomposition(
        const std::shared_ptr<AbstractTask> &task, FactOrder order,
        int random_seed);

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task,
        utils::LogProxy &log) const override;
};

/*
  Nest ModifiedGoalsTask and DomainAbstractedTask to return subtasks
  focussing on a single landmark fact.
*/
class LandmarkDecomposition : public SubtaskGenerator {
    FactOrder fact_order;
    bool combine_facts;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    /* Perform domain abstraction by combining facts that have to be
       achieved before a given landmark can be made true. */
    std::shared_ptr<AbstractTask> build_domain_abstracted_task(
        const std::shared_ptr<AbstractTask> &parent,
        const landmarks::LandmarkNode *node) const;

public:
    LandmarkDecomposition(
        const std::shared_ptr<AbstractTask> &task, FactOrder order,
        int random_seed, bool combine_facts);

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task,
        utils::LogProxy &log) const override;
};
}

#endif
