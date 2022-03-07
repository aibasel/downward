#ifndef LANDMARKS_EXPLORATION_H
#define LANDMARKS_EXPLORATION_H

#include "util.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
struct Proposition;
struct UnaryOperator;

struct Proposition {
    FactPair fact;
    // TODO: should we try to get rid of raw pointers?
    std::vector<UnaryOperator *> precondition_of;

    bool excluded;
    int h_max_cost;

    Proposition()
        : fact(FactPair::no_fact),
          excluded(false),
          h_max_cost(-1) {
    }

    bool operator<(const Proposition &other) const {
        return fact < other.fact;
    }
};

struct UnaryOperator {
    int op_or_axiom_id;
    // TODO; change to int num_preconditions
    std::vector<Proposition *> precondition;
    Proposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    bool excluded;
    int h_max_cost;
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                    int op_or_axiom_id, int base)
        : op_or_axiom_id(op_or_axiom_id), precondition(pre), effect(eff),
          base_cost(base), excluded(false) {}
};

class Exploration {
    TaskProxy task_proxy;

    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition>> propositions;
    // TODO: should we keep the raw pointer as type or rather an ID like in hmax?
    priority_queues::AdaptiveQueue<Proposition *> prop_queue;

    void build_unary_operators(const OperatorProxy &op);
    void setup_exploration_queue(
        const State &state, const std::vector<FactPair> &excluded_props,
        const std::unordered_set<int> &excluded_op_ids);
    void relaxed_exploration();
    void enqueue_if_necessary(Proposition *prop, int cost);
public:
    explicit Exploration(const TaskProxy &task_proxy);

    /*
      Computes the reachability of each proposition when excluding
      operators in *excluded_op_ids* and ensuring that propositions
      in *excluded_pros* are not achieved.
      The returned vector of vector denotes for each proposition
      (grouped by their fact variable) whether it is relaxed reachable.
      The values are exact in the absence of conditional effects,
      otherwise they are an admissible approximation.
    */
    // TODO: change vector<vector<bool>> to a proper class
    std::vector<std::vector<bool>> compute_relaxed_reachability(
        const std::vector<FactPair> &excluded_props,
        const std::unordered_set<int> &excluded_op_ids);
};
}

#endif
