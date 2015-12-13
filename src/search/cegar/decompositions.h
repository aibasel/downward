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
using Fact = std::pair<int, int>;
using Facts = std::vector<Fact>;
using Task = std::shared_ptr<AbstractTask>;
using Tasks = std::vector<Task>;

enum class SubtaskOrder {
    ORIGINAL,
    RANDOM,
    HADD_UP,
    HADD_DOWN
};


class Decomposition {
public:
    virtual Tasks get_subtasks(const Task &task) const = 0;
};


class NoDecomposition : public Decomposition {
    int num_copies;

public:
    explicit NoDecomposition(const Options &options);
    virtual ~NoDecomposition() = default;

    virtual Tasks get_subtasks(const Task &task) const override;
};


class FactDecomposition : public Decomposition {
    SubtaskOrder subtask_order;

    void remove_initial_state_facts(const TaskProxy &task_proxy, Facts &facts) const;
    void order_facts(const Task &task, Facts &facts) const;

protected:
    virtual Facts get_facts(const TaskProxy &task_proxy) const = 0;

    Facts get_filtered_and_ordered_facts(const Task &task) const;

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

    virtual Tasks get_subtasks(const Task &task) const override;
};


class LandmarkDecomposition : public FactDecomposition {
    const std::unique_ptr<Landmarks::LandmarkGraph> landmark_graph;
    bool combine_facts;

    Task get_domain_abstracted_task(Task parent, Fact fact) const;

protected:
    virtual Facts get_facts(const TaskProxy &) const override;

public:
    explicit LandmarkDecomposition(const Options &opts);
    virtual ~LandmarkDecomposition() = default;

    virtual Tasks get_subtasks(const Task &task) const override;
};
}

#endif
