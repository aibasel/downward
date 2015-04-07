#include "utils.h"

#include "../additive_heuristic.h"
#include "../option_parser.h"
#include "../task_proxy.h"
#include "../task_tools.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;

namespace cegar {
bool DEBUG = false;

// For values <= 50 MB the planner is often killed during the search.
// Reserving 75 MB avoids this.
static const int MEMORY_PADDING_MB = 75;

static char *cegar_memory_padding = 0;

// Save previous out-of-memory handler.
static void (*global_out_of_memory_handler)(void) = 0;

shared_ptr<AdditiveHeuristic> get_additive_heuristic(TaskProxy task) {
    Options opts;
    opts.set<TaskProxy *>("task_proxy", &task);
    opts.set<int>("cost_type", 0);
    shared_ptr<AdditiveHeuristic> additive_heuristic = make_shared<AdditiveHeuristic>(opts);
    additive_heuristic->initialize_and_compute_heuristic(task.get_initial_state());
    return additive_heuristic;
}

bool operator_applicable(OperatorProxy op, const unordered_set<FactProxy> &facts) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (facts.count(precondition) == 0)
            return false;
    }
    return true;
}

bool operator_achieves_fact(OperatorProxy op, FactProxy fact) {
    for (EffectProxy effect : op.get_effects()) {
        if (effect.get_fact() == fact)
            return true;
    }
    return false;
}

unordered_set<FactProxy> compute_possibly_before_facts(TaskProxy task, FactProxy last_fact) {
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

void get_unmet_preconditions(OperatorProxy op, const State &state, Splits *splits) {
    assert(splits->empty());
    for (FactProxy precondition : op.get_preconditions()) {
        VariableProxy var = precondition.get_variable();
        if (state[var] != precondition) {
            vector<int> wanted;
            wanted.push_back(precondition.get_value());
            splits->push_back(make_pair(var.get_id(), wanted));
        }
    }
    assert(splits->empty() == is_applicable(op, state));
}

void get_unmet_goals(GoalsProxy goals, const State &state, Splits *splits) {
    assert(splits->empty());
    for (FactProxy goal : goals) {
        VariableProxy var = goal.get_variable();
        if (state[var] != goal) {
            vector<int> wanted;
            wanted.push_back(goal.get_value());
            splits->push_back(make_pair(var.get_id(), wanted));
        }
    }
}

void continuing_out_of_memory_handler() {
    assert(cegar_memory_padding);
    delete[] cegar_memory_padding;
    cegar_memory_padding = 0;
    cout << "Failed to allocate memory for CEGAR abstraction. "
         << "Released memory padding and will stop refinement now." << endl;
    assert(global_out_of_memory_handler);
    set_new_handler(global_out_of_memory_handler);
}

void reserve_memory_padding() {
    assert(!cegar_memory_padding);
    if (DEBUG)
        cout << "Reserving " << MEMORY_PADDING_MB << " MB of memory padding." << endl;
    cegar_memory_padding = new char[MEMORY_PADDING_MB * 1024 * 1024];
    global_out_of_memory_handler = set_new_handler(continuing_out_of_memory_handler);
}

void release_memory_padding() {
    if (cegar_memory_padding) {
        delete[] cegar_memory_padding;
        cegar_memory_padding = 0;
    }
    assert(global_out_of_memory_handler);
    set_new_handler(global_out_of_memory_handler);
}

bool memory_padding_is_reserved() {
    return cegar_memory_padding != 0;
}
}
