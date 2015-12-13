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
    shared_ptr<AbstractTask> task) {
    Options opts;
    opts.set<shared_ptr<AbstractTask>>("transform", task);
    opts.set<int>("cost_type", 0);
    opts.set<bool>("cache_estimates", false);
    unique_ptr<AdditiveHeuristic::AdditiveHeuristic> additive_heuristic =
        Utils::make_unique_ptr<AdditiveHeuristic::AdditiveHeuristic>(opts);
    TaskProxy task_proxy(*task);
    additive_heuristic->initialize_and_compute_heuristic_for_cegar(
        task_proxy.get_initial_state());
    return additive_heuristic;
}

static bool operator_applicable(OperatorProxy op,
                                const unordered_set<FactProxy> &facts) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (facts.count(precondition) == 0)
            return false;
    }
    return true;
}

static bool operator_achieves_fact(OperatorProxy op, FactProxy fact) {
    for (EffectProxy effect : op.get_effects()) {
        if (effect.get_fact() == fact)
            return true;
    }
    return false;
}

static unordered_set<FactProxy> compute_possibly_before_facts(
    TaskProxy task, FactProxy last_fact) {
    unordered_set<FactProxy> pb_facts;

    // Add facts from initial state.
    for (FactProxy fact : task.get_initial_state())
        pb_facts.insert(fact);

    // Until no more facts can be added:
    size_t last_num_reached = 0;
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

unordered_set<FactProxy> get_relaxed_reachable_facts(TaskProxy task, FactProxy fact) {
    unordered_set<FactProxy> reachable_facts = compute_possibly_before_facts(task, fact);
    reachable_facts.insert(fact);
    return reachable_facts;
}

vector<int> get_domain_sizes(TaskProxy task) {
    vector<int> domain_sizes;
    for (VariableProxy var : task.get_variables())
        domain_sizes.push_back(var.get_domain_size());
    return domain_sizes;
}

int get_pre(OperatorProxy op, int var_id) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (precondition.get_variable().get_id() == var_id)
            return precondition.get_value();
    }
    return UNDEFINED;
}

int get_eff(OperatorProxy op, int var_id) {
    for (EffectProxy effect : op.get_effects()) {
        if (effect.get_fact().get_variable().get_id() == var_id)
            return effect.get_fact().get_value();
    }
    return UNDEFINED;
}

int get_post(OperatorProxy op, int var_id) {
    int eff = get_eff(op, var_id);
    if (eff != UNDEFINED)
        return eff;
    return get_pre(op, var_id);
}
}
