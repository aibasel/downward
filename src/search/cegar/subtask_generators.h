#ifndef CEGAR_SUBTASK_GENERATORS_H
#define CEGAR_SUBTASK_GENERATORS_H

#include <memory>
#include <utility>
#include <vector>

class AbstractTask;
struct Fact;
class TaskProxy;

namespace landmarks {
class LandmarkGraph;
}

namespace options {
class Options;
}

namespace cegar {
using Facts = std::vector<Fact>;
using SharedTasks = std::vector<std::shared_ptr<AbstractTask>>;

enum class FactOrder {
    ORIGINAL,
    RANDOM,
    HADD_UP,
    HADD_DOWN
};


class SubtaskGenerator {
public:
    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const = 0;
};


class TaskDuplicator : public SubtaskGenerator {
    int num_copies;

public:
    explicit TaskDuplicator(const options::Options &options);
    virtual ~TaskDuplicator() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};


class GoalDecomposition : public SubtaskGenerator {
    FactOrder fact_order;

    Facts get_goal_facts(const TaskProxy &task_proxy) const;

public:
    explicit GoalDecomposition(const options::Options &options);
    virtual ~GoalDecomposition() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};


class LandmarkDecomposition : public SubtaskGenerator {
    FactOrder fact_order;
    const std::unique_ptr<landmarks::LandmarkGraph> landmark_graph;
    bool combine_facts;

    /* Perform domain abstraction by combining facts that have to be
       achieved before a given landmark can be made true. */
    std::shared_ptr<AbstractTask> build_domain_abstracted_task(
        std::shared_ptr<AbstractTask> &parent, Fact fact) const;

public:
    explicit LandmarkDecomposition(const options::Options &opts);
    virtual ~LandmarkDecomposition();

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
