#ifndef PRUNING_STUBBORN_SETS_QUEUE_H
#define PRUNING_STUBBORN_SETS_QUEUE_H

#include "stubborn_sets.h"

namespace stubborn_sets_queue {
static const int MARKED_VALUES_NONE = -2;
static const int MARKED_VALUES_ALL = -1;

enum class VariableOrdering {
    FAST_DOWNWARD,
    MINIMIZE_SS,
    STATIC_SMALL,
    DYNAMIC_SMALL,
};

class StubbornSetsQueue : public stubborn_sets::StubbornSets {
    const bool mark_variables;
    const VariableOrdering variable_ordering;

    // Operator IDs that contain the fact in their precondition.
    std::vector<std::vector<std::vector<int>>> consumers;
    // Marked producer and consumer facts.
    std::vector<std::vector<bool>> marked_producers;
    std::vector<std::vector<bool>> marked_consumers;
    // Marked producer and consumer variables (marked iff whole domain is marked).
    std::vector<int> marked_producer_variables;
    std::vector<int> marked_consumer_variables;
    std::vector<FactPair> producer_queue;
    std::vector<FactPair> consumer_queue;

    void compute_consumers(const TaskProxy &task_proxy);
    bool operator_is_applicable(int op, const State &state);
    void enqueue_producers(const FactPair &fact);
    void enqueue_consumers(const FactPair &fact);
    void enqueue_sibling_consumers(const FactPair &fact);
    void enqueue_sibling_producers(const FactPair &fact);
    FactPair select_fact(const std::vector<FactPair> &facts, const State &state) const;
    void enqueue_nes(int op, const State &state);
    void enqueue_interferers(int op);
protected:
    virtual void initialize_stubborn_set(const State &state) override;
    virtual void handle_stubborn_operator(const State &state, int op) override;
public:
    explicit StubbornSetsQueue(const options::Options &opts);

    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
