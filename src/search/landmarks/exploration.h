#ifndef LANDMARKS_EXPLORATION_H
#define LANDMARKS_EXPLORATION_H

#include "util.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"

#include <vector>

namespace utils {
class LogProxy;
}

namespace landmarks {
struct Proposition;
struct UnaryOperator;

struct Proposition {
    FactPair fact;
    std::vector<UnaryOperator *> precondition_of;
    bool reached;
    bool excluded;

    Proposition()
        : fact(FactPair::no_fact),
          reached(false),
          excluded(false) {
    }

    bool operator<(const Proposition &other) const {
        return fact < other.fact;
    }
};

struct UnaryOperator {
    int op_or_axiom_id;
    const int num_preconditions;
    Proposition *effect;

    int unsatisfied_preconditions;
    bool excluded;
    UnaryOperator(const std::vector<Proposition *> &preconditions,
                  Proposition *eff, int op_or_axiom_id)
        : op_or_axiom_id(op_or_axiom_id),
          num_preconditions(static_cast<int>(preconditions.size())),
          effect(eff),
          unsatisfied_preconditions(num_preconditions),
          excluded(false) {}
};

class Exploration {
    TaskProxy task_proxy;

    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition>> propositions;
    std::deque<Proposition *> prop_queue;

    void build_unary_operators(const OperatorProxy &op);
    void setup_exploration_queue(
        const State &state, const std::vector<FactPair> &excluded_props,
        bool use_unary_relaxation);
    void relaxed_exploration();
    void enqueue_if_necessary(Proposition *prop);
public:
    Exploration(const TaskProxy &task_proxy, utils::LogProxy &log);

    /*
      Computes the reachability of each proposition when excluding
      operators in *excluded_op_ids* and ensuring that propositions
      in *excluded_pros* are not achieved.
      The returned vector of vector denotes for each proposition
      (grouped by their fact variable) whether it is relaxed reachable.
      The values are exact in the absence of conditional effects, otherwise
      they are an admissible approximation (see implementation for details).
    */
    std::vector<std::vector<bool>> compute_relaxed_reachability(
        const std::vector<FactPair> &excluded_props, bool use_unary_relaxation);
};
}

#endif
