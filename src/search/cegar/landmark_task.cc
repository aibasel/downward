#include "landmark_task.h"

#include "values.h"

#include "../task_proxy.h"
#include "../task_tools.h"
#include "../utilities.h"

#include <algorithm>
#include <set>

using namespace std;

namespace cegar {

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

unordered_set<FactProxy> compute_reachable_facts(TaskProxy task, FactProxy landmark) {
    unordered_set<FactProxy> reachable_facts = compute_possibly_before_facts(task, landmark);
    reachable_facts.insert(landmark);
    return reachable_facts;
}

LandmarkTask::LandmarkTask(shared_ptr<AbstractTask> parent,
                           FactProxy landmark,
                           const VariableToValues &fact_groups)
    : DelegatingTask(parent),
      initial_state_data(parent->get_initial_state_values()),
      goals({get_raw_fact(landmark)}),
      task_index(parent->get_num_variables()) {
    TaskProxy orig_task = TaskProxy(parent.get());
    assert(variable_domain.empty());
    for (VariableProxy var : orig_task.get_variables()) {
        variable_domain.push_back(var.get_domain_size());
    }
    fact_names = g_fact_names;
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        task_index[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            task_index[var][value] = value;
        }
    }
    for (const auto &group : fact_groups) {
        if (group.second.size() >= 2)
            combine_facts(group.first, group.second);
    }
}

void LandmarkTask::move_fact(int var, int before, int after) {
    if (DEBUG)
        cout << "Move fact " << var << ": " << before << " -> " << after << endl;
    assert(in_bounds(before, task_index[var]));
    assert(in_bounds(after, task_index[var]));
    assert(task_index[var][before] == before);
    task_index[var][before] = after;
    //fact_names[var][after] = fact_names[var][before];
    if (initial_state_data[var] == before)
        initial_state_data[var] = after;
    for (Fact &goal : goals) {
        if (var == goal.first && before == goal.second)
            goal.second = after;
    }
}

void LandmarkTask::combine_facts(int var, const unordered_set<int> &values) {
    assert(values.size() >= 2);
    if (DEBUG)
        cout << "Combine " << var << ": mapped " << to_string(values) << endl;
    int group_value = *values.begin();
    for (int value : values) {
        move_fact(var, value, group_value);
    }

    // Don't decrease the variable's domain size. The FactProxy class expects
    // all values to be smaller than the domain size (i.e., sequentially numbered).

    // Set combined fact name.
    stringstream name;
    string sep = "";
    for (int value : values) {
        name << sep << fact_names[var][value];
        sep = " OR ";
    }
    fact_names[var][group_value] = name.str();
}

void LandmarkTask::dump_facts() const {
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        for (int value = 0; value < variable_domain[var]; ++value)
            cout << "    " << var << "=" << value << ": " << fact_names[var][value] << endl;
    }
}

void LandmarkTask::dump_name() const {
    cout << "Task ";
    string sep = "";
    for (const Fact &goal : goals) {
        cout << sep << goal.first << "=" << goal.second
             << ":" << fact_names[goal.first][goal.second];
        sep = " ";
    }
    cout << endl;
}

void LandmarkTask::dump() const {
    dump_name();
    int num_facts = 0;
    for (size_t var = 0; var < variable_domain.size(); ++var)
        num_facts += variable_domain[var];
    cout << "  Facts: " << num_facts << endl;
    if (DEBUG)
        dump_facts();
    cout << "  " << "Variable domain sizes: " << to_string(variable_domain) << endl;
    if (DEBUG) {
        cout << "  " << "Fact mapping:" << endl;
        for (size_t var = 0; var < task_index.size(); ++var)
            cout << "    " << var << ": " << to_string(task_index[var]) << endl;
    }
}

int LandmarkTask::get_variable_domain_size(int var) const {
    return variable_domain[var];
}

const string &LandmarkTask::get_fact_name(int var, int value) const {
    return fact_names[var][value];
}

pair<int, int> LandmarkTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return get_task_fact(parent->get_operator_precondition(op_index, fact_index, is_axiom));
}

pair<int, int> LandmarkTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_task_fact(parent->get_operator_effect(op_index, eff_index, is_axiom));
}

int LandmarkTask::get_num_goals() const {
    return goals.size();
}

pair<int, int> LandmarkTask::get_goal_fact(int index) const {
    return goals[index];
}

std::vector<int> LandmarkTask::get_initial_state_values() const {
    return initial_state_data;
}

vector<int> LandmarkTask::get_state_values(const GlobalState &global_state) const {
    int num_vars = variable_domain.size();
    vector<int> state_data(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int value = task_index[var][global_state[var]];
        assert(value != UNDEFINED);
        state_data[var] = value;
    }
    return state_data;
}
}
