#include "utils.h"

#include "../option_parser.h"
#include "../task_proxy.h"
#include "../task_tools.h"

#include "../heuristics/additive_heuristic.h"

#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>

using namespace std;

namespace CEGAR {
bool DEBUG = false;

unique_ptr<AdditiveHeuristic::AdditiveHeuristic> get_additive_heuristic(
    const shared_ptr<AbstractTask> &task) {
    Options opts;
    opts.set<shared_ptr<AbstractTask>>("transform", task);
    opts.set<int>("cost_type", NORMAL);
    opts.set<bool>("cache_estimates", false);
    return Utils::make_unique_ptr<AdditiveHeuristic::AdditiveHeuristic>(opts);
}

static bool operator_applicable(
    const OperatorProxy &op, const unordered_set<FactProxy> &facts) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (facts.count(precondition) == 0)
            return false;
    }
    return true;
}

static bool operator_achieves_fact(
    const OperatorProxy &op, const FactProxy &fact) {
    for (EffectProxy effect : op.get_effects()) {
        if (effect.get_fact() == fact)
            return true;
    }
    return false;
}

static unordered_set<FactProxy> compute_possibly_before_facts(
    const TaskProxy &task, const FactProxy &last_fact) {
    unordered_set<FactProxy> pb_facts;

    // Add facts from initial state.
    for (FactProxy fact : task.get_initial_state())
        pb_facts.insert(fact);

    // Until no more facts can be added:
    size_t last_num_reached = 0;
    /*
      Note: This can be done more efficiently by maintaining the number
      of unsatisfied preconditions for each operator and a queue of
      unhandled effects.

      TODO: Find out if this code is time critical, and change it if it
      is.
    */
    while (last_num_reached != pb_facts.size()) {
        last_num_reached = pb_facts.size();
        for (OperatorProxy op : task.get_operators()) {
            // Ignore operators that achieve last_fact.
            if (operator_achieves_fact(op, last_fact))
                continue;
            // Add all facts that are achieved by an applicable operator.
            if (operator_applicable(op, pb_facts)) {
                for (EffectProxy effect : op.get_effects()) {
                    pb_facts.insert(effect.get_fact());
                }
            }
        }
    }
    return pb_facts;
}

unordered_set<FactProxy> get_relaxed_possible_before(
    const TaskProxy &task, const FactProxy &fact) {
    unordered_set<FactProxy> reachable_facts =
        compute_possibly_before_facts(task, fact);
    reachable_facts.insert(fact);
    return reachable_facts;
}

vector<int> get_domain_sizes(const TaskProxy &task) {
    vector<int> domain_sizes;
    for (VariableProxy var : task.get_variables())
        domain_sizes.push_back(var.get_domain_size());
    return domain_sizes;
}

int get_pre(const OperatorProxy &op, int var_id) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (precondition.get_variable().get_id() == var_id)
            return precondition.get_value();
    }
    return UNDEFINED;
}

int get_eff(const OperatorProxy &op, int var_id) {
    for (EffectProxy effect : op.get_effects()) {
        if (effect.get_fact().get_variable().get_id() == var_id)
            return effect.get_fact().get_value();
    }
    return UNDEFINED;
}

int get_post(const OperatorProxy &op, int var_id) {
    int eff = get_eff(op, var_id);
    if (eff != UNDEFINED)
        return eff;
    return get_pre(op, var_id);
}
}
