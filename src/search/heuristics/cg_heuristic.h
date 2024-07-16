#ifndef HEURISTICS_CG_HEURISTIC_H
#define HEURISTICS_CG_HEURISTIC_H

#include "../heuristic.h"

#include "../algorithms/priority_queues.h"
#include "../tasks/default_value_axioms_task.h"

#include <memory>
#include <string>
#include <vector>

namespace domain_transition_graph {
class DomainTransitionGraph;
struct ValueNode;
}

namespace cg_heuristic {
class CGCache;

class CGHeuristic : public Heuristic {
    using ValueNodeQueue = priority_queues::AdaptiveQueue<domain_transition_graph::ValueNode *>;
    std::vector<std::unique_ptr<ValueNodeQueue>> prio_queues;
    std::vector<std::unique_ptr<domain_transition_graph::DomainTransitionGraph>> transition_graphs;

    std::unique_ptr<CGCache> cache;
    int cache_hits;
    int cache_misses;

    int helpful_transition_extraction_counter;

    int min_action_cost;

    void setup_domain_transition_graphs();
    int get_transition_cost(
        const State &state,
        domain_transition_graph::DomainTransitionGraph *dtg,
        int start_val,
        int goal_val);
    void mark_helpful_transitions(
        const State &state,
        domain_transition_graph::DomainTransitionGraph *dtg,
        int to);
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit CGHeuristic(
        int max_cache_size,
        tasks::AxiomHandlingType axiom_hanlding,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
