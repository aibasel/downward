#ifndef LANDMARKS_EXPLORATION_H
#define LANDMARKS_EXPLORATION_H

#include "../heuristic.h"
#include "../globals.h"
#include "../priority_queue.h"
#include "landmark_types.h"

#include <cassert>
#include <unordered_map>
#include <vector>

class GlobalOperator;
class GlobalState;

struct ExProposition;
struct ExUnaryOperator;

struct ExProposition {
    int var;
    int val;
    bool is_goal_condition;
    bool is_termination_condition;
    std::vector<ExUnaryOperator *> precondition_of;

    int h_add_cost;
    int h_max_cost;
    int depth;
    bool marked; // used when computing preferred operators
    ExUnaryOperator *reached_by;

    ExProposition() {
        is_goal_condition = false;
        is_termination_condition = false;
        h_add_cost = -1;
        h_max_cost = -1;
        reached_by = 0;
        marked = false;
    }

    bool operator<(const ExProposition &other) const {
        return var < other.var || (var == other.var && val < other.val);
    }
};

struct ExUnaryOperator {
    const GlobalOperator *op;
    std::vector<ExProposition *> precondition;
    ExProposition *effect;
    int base_cost; // 0 for axioms, 1 for regular operators

    int unsatisfied_preconditions;
    int h_add_cost;
    int h_max_cost;
    int depth;
    ExUnaryOperator(const std::vector<ExProposition *> &pre, ExProposition *eff,
                    const GlobalOperator *the_op, int base)
        : op(the_op), precondition(pre), effect(eff), base_cost(base) {}


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

class Exploration : public Heuristic {
    static const int MAX_COST_VALUE = 100000000; // See additive_heuristic.h.

    typedef std::unordered_set<const GlobalOperator *> RelaxedPlan;
    RelaxedPlan relaxed_plan;
    std::vector<ExUnaryOperator> unary_operators;
    std::vector<std::vector<ExProposition> > propositions;
    std::vector<ExProposition *> goal_propositions;
    std::vector<ExProposition *> termination_propositions;

    AdaptiveQueue<ExProposition *> prop_queue;
    bool did_write_overflow_warning;

    bool heuristic_recomputation_needed;

    void build_unary_operators(const GlobalOperator &op);
    void simplify();

    void setup_exploration_queue(const GlobalState &state,
                                 const std::vector<std::pair<int, int> > &excluded_props,
                                 const std::unordered_set<const GlobalOperator *> &excluded_ops,
                                 bool use_h_max);
    inline void setup_exploration_queue(const GlobalState &state, bool h_max) {
        std::vector<std::pair<int, int> > excluded_props;
        std::unordered_set<const GlobalOperator *> excluded_ops;
        setup_exploration_queue(state, excluded_props, excluded_ops, h_max);
    }
    void relaxed_exploration(bool use_h_max, bool level_out);
    void prepare_heuristic_computation(const GlobalState &state, bool h_max);
    void collect_relaxed_plan(ExProposition *goal, RelaxedPlan &relaxed_plan, const GlobalState &state);

    int compute_hsp_add_heuristic();
    int compute_ff_heuristic(const GlobalState &state);

    void collect_ha(ExProposition *goal, RelaxedPlan &relaxed_plan, const GlobalState &state);

    void enqueue_if_necessary(ExProposition *prop, int cost, int depth, ExUnaryOperator *op,
                              bool use_h_max);
    void increase_cost(int &cost, int amount);
    void write_overflow_warning();
protected:
    virtual int compute_heuristic(const GlobalState &state);
public:
    void set_additional_goals(const std::vector<std::pair<int, int> > &goals);
    void set_recompute_heuristic() {heuristic_recomputation_needed = true; }
    void compute_reachability_with_excludes(std::vector<std::vector<int> > &lvl_var,
                                            std::vector<std::unordered_map<std::pair<int, int>, int> > &lvl_op,
                                            bool level_out,
                                            const std::vector<std::pair<int, int> > &excluded_props,
                                            const std::unordered_set<const GlobalOperator *> &excluded_ops,
                                            bool compute_lvl_ops);
    std::vector<const GlobalOperator *> exported_ops; // only needed for landmarks count heuristic ha

    // Returns true iff disj_goal is relaxed reachable. As a side effect, marks preferred operators
    // via "exported_ops". (This is the real reason why you might want to call this.)
    bool plan_for_disj(std::vector<std::pair<int, int> > &disj_goal, const GlobalState &state);

    Exploration(const Options &opts);
    ~Exploration();
};

#endif
