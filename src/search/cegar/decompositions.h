#ifndef CEGAR_DECOMPOSITIONS_H
#define CEGAR_DECOMPOSITIONS_H

#include <memory>
#include <utility>
#include <vector>

class AbstractTask;
class Options;
class TaskProxy;

namespace Landmarks {
class LandmarkGraph;
}

namespace CEGAR {
// TODO: Remove typedef.
using Fact = std::pair<int, int>;
using Facts = std::vector<Fact>;
using SharedTasks = std::vector<std::shared_ptr<AbstractTask>>;

enum class SubtaskOrder {
    ORIGINAL,
    RANDOM,
    HADD_UP,
    HADD_DOWN
};


class Decomposition {
public:
    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const = 0;
};


class NoDecomposition : public Decomposition {
    int num_copies;

public:
    explicit NoDecomposition(const Options &options);
    virtual ~NoDecomposition() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};


class GoalDecomposition : public Decomposition {
    SubtaskOrder subtask_order;

    Facts get_goal_facts(const TaskProxy &task_proxy) const;

public:
    explicit GoalDecomposition(const Options &options);
    virtual ~GoalDecomposition() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};


class LandmarkDecomposition : public Decomposition {
    SubtaskOrder subtask_order;
    const std::unique_ptr<Landmarks::LandmarkGraph> landmark_graph;
    bool combine_facts;

    std::shared_ptr<AbstractTask> get_domain_abstracted_task(
        std::shared_ptr<AbstractTask> &parent, Fact fact) const;

public:
    explicit LandmarkDecomposition(const Options &opts);
    virtual ~LandmarkDecomposition() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
