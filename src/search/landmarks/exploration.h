#ifndef LANDMARKS_EXPLORATION_H
#define LANDMARKS_EXPLORATION_H

#include "util.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace utils {
class LogProxy;
}

namespace landmarks {
struct ExProposition;
struct ExUnaryOperator;

struct ExProposition {
    FactPair fact;
    std::vector<ExUnaryOperator *> precondition_of;

    bool excluded;
    int h_max_cost;

    ExProposition()
        : fact(FactPair::no_fact),
          excluded(false),
          h_max_cost(-1) {
    }

    bool operator<(const ExProposition &other) const {
        return fact < other.fact;
    }
};

struct ExUnaryOperator {
    int op_or_axiom_id;
    std::vector<ExProposition *> precondition;
    ExProposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    bool excluded;
    int h_max_cost;
    ExUnaryOperator(const std::vector<ExProposition *> &pre, ExProposition *eff,
                    int op_or_axiom_id, int base)
        : op_or_axiom_id(op_or_axiom_id), precondition(pre), effect(eff),
          base_cost(base), excluded(false) {}


    bool operator<(const ExUnaryOperator &other) const {
        if (*(other.effect) < *effect)
            return false;
        else if (*effect < *(other.effect))
            return true;

        else {
            for (size_t i = 0; i < precondition.size(); ++i) {
                if (i == other.precondition.size() ||
                    *(other.precondition[i]) < *(precondition[i]))
                    return false;
                else if (*(precondition[i]) < *(other.precondition[i]))
                    return true;
            }
            return true;
        }
    }
};

class Exploration {
    TaskProxy task_proxy;

    std::vector<ExUnaryOperator> unary_operators;
    std::vector<std::vector<ExProposition>> propositions;
    std::vector<ExProposition *> goal_propositions;
    std::vector<ExProposition *> termination_propositions;
    priority_queues::AdaptiveQueue<ExProposition *> prop_queue;

    void build_unary_operators(const OperatorProxy &op);
    void setup_exploration_queue(
        const State &state, const std::vector<FactPair> &excluded_props,
        const std::unordered_set<int> &excluded_op_ids);
    void relaxed_exploration();
    void enqueue_if_necessary(ExProposition *prop, int cost);
public:
    Exploration(const TaskProxy &task_proxy, utils::LogProxy &log);

    /*
      Computes the h_max costs (stored in *lvl_var*) for the problem when
      excluding propositions in *excluded_props* and operators in
      *excluded_op_ids*. The values are only exact in the absence of conditional
      effects, otherwise they are an admissible approximation.
    */
    void compute_reachability_with_excludes(
        std::vector<std::vector<int>> &lvl_var,
        const std::vector<FactPair> &excluded_props,
        const std::unordered_set<int> &excluded_op_ids);
};
}

#endif
