#ifndef CEGAR_DECOMPOSITIONS_H
#define CEGAR_DECOMPOSITIONS_H

#include "../additive_heuristic.h"
#include "../option_parser.h"
#include "../task_proxy.h"

#include <memory>
#include <utility>
#include <vector>

namespace cegar {
using Fact = std::pair<int, int>;
using Facts = std::vector<Fact>;
using Subtask = std::shared_ptr<AbstractTask>;
using Subtasks = std::vector<Subtask>;

enum class SubtaskOrder {
    ORIGINAL,
    MIXED,
    HADD_UP,
    HADD_DOWN
};


class Decomposition {
    const std::shared_ptr<AbstractTask> task;

protected:
    const TaskProxy task_proxy;

    Subtask get_original_task() const;

public:
    explicit Decomposition(const Options &options);
    virtual ~Decomposition() = default;

    virtual Subtasks get_subtasks() const = 0;
};


class NoDecomposition : public Decomposition {
    int num_copies;

public:
    explicit NoDecomposition(const Options &options);
    virtual ~NoDecomposition() = default;

    virtual Subtasks get_subtasks() const override;
};


class FactDecomposition : public Decomposition {
    SubtaskOrder subtask_order;

    struct SortHaddValuesUp {
        const std::shared_ptr<AdditiveHeuristic> hadd;

        explicit SortHaddValuesUp(std::shared_ptr<AdditiveHeuristic> hadd)
            : hadd(hadd) {
        }

        int get_cost(Fact fact) {
            return hadd->get_cost(fact.first, fact.second);
        }

        bool operator()(Fact a, Fact b) {
            return get_cost(a) < get_cost(b);
        }
    };

    void remove_initial_state_facts(Facts &facts) const;
    void order_facts(Facts &facts) const;

protected:
    virtual Facts get_facts() const = 0;

    Facts get_filtered_and_ordered_facts() const;

public:
    explicit FactDecomposition(const Options &options);
    virtual ~FactDecomposition() = default;
};


class GoalDecomposition : public FactDecomposition {
protected:
    virtual Facts get_facts() const override;

public:
    explicit GoalDecomposition(const Options &options);
    virtual ~GoalDecomposition() = default;

    virtual Subtasks get_subtasks() const override;
};


class LandmarkDecomposition : public FactDecomposition {
    const std::shared_ptr<LandmarkGraph> landmark_graph;
    bool combine_facts;

    Subtask get_domain_abstracted_task(Subtask parent, Fact fact) const;

protected:
    virtual Facts get_facts() const override;

public:
    explicit LandmarkDecomposition(const Options &opts);
    virtual ~LandmarkDecomposition() = default;

    virtual Subtasks get_subtasks() const override;
};
}

#endif
