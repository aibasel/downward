#ifndef PRUNING_STUBBORN_SETS_QUEUE_H
#define PRUNING_STUBBORN_SETS_QUEUE_H

#include "stubborn_sets.h"

namespace stubborn_sets_queue {
class StubbornSetsQueue : public stubborn_sets::StubbornSets {
    std::vector<std::vector<std::vector<int>>> consumers;
    std::vector<std::vector<bool>> marked_producers;
    std::vector<std::vector<bool>> marked_consumers;
    std::vector<FactPair> producer_queue;
    std::vector<FactPair> consumer_queue;

    void compute_consumers(const TaskProxy &task_proxy);
    bool operator_is_applicable(int op, const State &state);
    void enqueue_producers(const FactPair &fact);
    void enqueue_consumers(const FactPair &fact);
    void enqueue_sibling_consumers(const FactPair &fact);
    void enqueue_sibling_producers(const FactPair &fact);
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
