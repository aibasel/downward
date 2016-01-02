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


class FactDecomposition : public Decomposition {
    SubtaskOrder subtask_order;

    void remove_initial_state_facts(const TaskProxy &task_proxy, Facts &facts) const;
    void order_facts(const std::shared_ptr<AbstractTask> &task, Facts &facts) const;

protected:
    virtual Facts get_facts(const TaskProxy &task_proxy) const = 0;

    Facts get_filtered_and_ordered_facts(
        const std::shared_ptr<AbstractTask> &task) const;

public:
    explicit FactDecomposition(const Options &options);
    virtual ~FactDecomposition() = default;
};


class GoalDecomposition : public FactDecomposition {
protected:
    virtual Facts get_facts(const TaskProxy &task_proxy) const override;

public:
    explicit GoalDecomposition(const Options &options);
    virtual ~GoalDecomposition() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};


class LandmarkDecomposition : public FactDecomposition {
    const std::unique_ptr<Landmarks::LandmarkGraph> landmark_graph;
    bool combine_facts;

    std::shared_ptr<AbstractTask> get_domain_abstracted_task(
        std::shared_ptr<AbstractTask> &parent, Fact fact) const;

protected:
    virtual Facts get_facts(const TaskProxy &) const override;

public:
    explicit LandmarkDecomposition(const Options &opts);
    virtual ~LandmarkDecomposition() = default;

    virtual SharedTasks get_subtasks(
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
