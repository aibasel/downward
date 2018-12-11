#ifndef LANDMARKS_EXPLORATION_H
#define LANDMARKS_EXPLORATION_H

#include "util.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
struct ExProposition;
struct ExUnaryOperator;

struct ExProposition {
    FactPair fact;
    bool is_goal_condition;
    bool is_termination_condition;
    std::vector<ExUnaryOperator *> precondition_of;

    int h_add_cost;
    int h_max_cost;
    int depth;
    bool marked; // used when computing preferred operators
    ExUnaryOperator *reached_by;

    ExProposition()
        : fact(FactPair::no_fact),
          is_goal_condition(false),
          is_termination_condition(false),
          h_add_cost(-1),
          h_max_cost(-1),
          depth(-1),
          marked(false),
          reached_by(nullptr)
    {}

    bool operator<(const ExProposition &other) const {
        return fact < other.fact;
    }
};

struct ExUnaryOperator {
    int op_or_axiom_id;
    std::vector<ExProposition *> precondition;
    ExProposition *effect;
    int base_cost; // 0 for axioms, 1 for regular operators

    int unsatisfied_preconditions;
    int h_add_cost;
    int h_max_cost;
    int depth;
    ExUnaryOperator(const std::vector<ExProposition *> &pre, ExProposition *eff,
                    int op_or_axiom_id, int base)
        : op_or_axiom_id(op_or_axiom_id), precondition(pre), effect(eff), base_cost(base) {}


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

    bool is_induced_by_axiom(const TaskProxy &task_proxy) const {
        return get_operator_or_axiom(task_proxy, op_or_axiom_id).is_axiom();
    }
};

class Exploration {
    static const int MAX_COST_VALUE = 100000000; // See additive_heuristic.h.

    TaskProxy task_proxy;

    std::vector<ExUnaryOperator> unary_operators;
    std::vector<std::vector<ExProposition>> propositions;
    std::vector<ExProposition *> goal_propositions;
    std::vector<ExProposition *> termination_propositions;
    priority_queues::AdaptiveQueue<ExProposition *> prop_queue;
    bool did_write_overflow_warning;

    void build_unary_operators(const OperatorProxy &op);
    void setup_exploration_queue(const State &state,
                                 const std::vector<FactPair> &excluded_props,
                                 const std::unordered_set<int> &excluded_op_ids,
                                 bool use_h_max);
    void relaxed_exploration(bool use_h_max, bool level_out);
    void enqueue_if_necessary(ExProposition *prop, int cost, int depth, ExUnaryOperator *op,
                              bool use_h_max);
    void increase_cost(int &cost, int amount);
    void write_overflow_warning();
public:
    explicit Exploration(const TaskProxy &task_proxy);

    void compute_reachability_with_excludes(std::vector<std::vector<int>> &lvl_var,
                                            std::vector<utils::HashMap<FactPair, int>> &lvl_op,
                                            bool level_out,
                                            const std::vector<FactPair> &excluded_props,
                                            const std::unordered_set<int> &excluded_op_ids,
                                            bool compute_lvl_ops);
};
}

#endif
