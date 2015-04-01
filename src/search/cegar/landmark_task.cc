#include "landmark_task.h"

#include "values.h"

#include "../globals.h"
#include "../state_registry.h"
#include "../task_proxy.h"
#include "../task_tools.h"
#include "../timer.h"
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
      unreachable_facts(parent->get_num_variables()),
      orig_index(parent->get_num_variables()),
      task_index(parent->get_num_variables()) {
    TaskProxy orig_task = TaskProxy(parent.get());
    assert(variable_domain.empty());
    for (VariableProxy var : orig_task.get_variables()) {
        variable_domain.push_back(var.get_domain_size());
    }
    fact_names = g_fact_names;
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        orig_index[var].resize(variable_domain[var]);
        task_index[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            orig_index[var][value] = value;
            task_index[var][value] = value;
        }
    }
    for (const auto &group : fact_groups) {
        if (group.second.size() >= 2)
            combine_facts(group.first, group.second);
    }
}

LandmarkTask::LandmarkTask(vector<int> domain, vector<vector<string> > names,
           vector<int> initial_state_data_, vector<Fact> goal_facts)
    : DelegatingTask(g_root_task()),
      initial_state_data(initial_state_data_),
      goals(goal_facts),
      variable_domain(domain),
      unreachable_facts(domain.size()),
      fact_names(names),
      orig_index(domain.size()),
      task_index(domain.size()) {
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        orig_index[var].resize(variable_domain[var]);
        task_index[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            orig_index[var][value] = value;
            task_index[var][value] = value;
        }
    }
}

int LandmarkTask::get_orig_op_index(int index) const {
    // TODO: Update this if we ever drop operators.
    return index;
}

void LandmarkTask::move_fact(int var, int before, int after) {
    if (DEBUG)
        cout << "Move fact " << var << ": " << before << " -> " << after << endl;
    assert(in_bounds(before, task_index[var]));
    if (after == UNDEFINED) {
        assert(initial_state_data[var] != before);
        for (size_t i = 0; i < goals.size(); ++i) {
            assert(goals[i].first != var || goals[i].second != before);
        }
        task_index[var][orig_index[var][before]] = UNDEFINED;
        return;
    }
    assert(in_bounds(after, task_index[var]));
    // We never move a fact with more than one original index. If we did, we
    // would have to use a vector here.
    orig_index[var][after] = orig_index[var][before];
    assert(orig_index[var][before] == before);
    task_index[var][orig_index[var][before]] = after;
    fact_names[var][after] = fact_names[var][before];
    if (initial_state_data[var] == before)
        initial_state_data[var] = after;
    for (size_t i = 0; i < goals.size(); ++i) {
        if (var == goals[i].first && before == goals[i].second)
            goals[i].second = after;
    }
    // TODO: Do we still have to do this?
    unordered_set<int>::iterator it = unreachable_facts[var].find(before);
    if (it != unreachable_facts[var].end()) {
        unreachable_facts[var].erase(it);
        unreachable_facts[var].insert(after);
    }
}

void LandmarkTask::update_facts(int var, int num_values, const vector<int> &new_task_index) {
    assert(num_values >= 1);
    int num_indices = new_task_index.size();
    assert(num_indices == variable_domain[var]);
    for (int before = 0; before < num_indices; ++before) {
        int after = new_task_index[before];
        assert(before >= after);
        assert(after != -2);
        if (before != after) {
            move_fact(var, before, after);
        }
    }
    orig_index[var].erase(orig_index[var].begin() + num_values, orig_index[var].end());
    fact_names[var].erase(fact_names[var].begin() + num_values, fact_names[var].end());
    if (DEBUG && num_values < variable_domain[var]) {
        cout << "Task index " << var << ": " << to_string(task_index[var]) << endl;
        cout << "Orig index " << var << ": " << to_string(orig_index[var]) << endl;
    }
    variable_domain[var] = num_values;
    assert(static_cast<int>(orig_index[var].size()) == num_values);
    assert(static_cast<int>(fact_names[var].size()) == num_values);
}

void LandmarkTask::find_and_apply_new_fact_ordering(int var, set<int> &unordered_values, int value_for_rest) {
    // Save renamings by filling the free indices with facts from the back.
    assert(!unordered_values.empty());
    int num_values = unordered_values.size();
    vector<int> new_task_index(variable_domain[var], -2);
    for (int value = 0; value < variable_domain[var]; ++value) {
        if (new_task_index[value] != -2) {
            // We already found a position for value.
        } else if (unordered_values.count(value) == 1) {
            new_task_index[value] = value;
            unordered_values.erase(unordered_values.find(value));
        } else {
            new_task_index[value] = value_for_rest;
            if (!unordered_values.empty()) {
                int highest_value = *unordered_values.rbegin();
                new_task_index[highest_value] = value;
                unordered_values.erase(highest_value);
            }
        }
    }
    assert(unordered_values.empty());
    update_facts(var, num_values, new_task_index);
}

void LandmarkTask::combine_facts(int var, const unordered_set<int> &values) {
    assert(values.size() >= 2);
    set<int> mapped_values;
    for (int value : values) {
        assert(task_index[var][value] != UNDEFINED);
        mapped_values.insert(get_task_value(var, value));
    }
    if (DEBUG)
        cout << "Combine " << var << ": mapped " << to_string(mapped_values) << endl;
    int projected_value = *mapped_values.begin();
    set<int> kept_values;
    for (int value = 0; value < variable_domain[var]; ++value) {
        if (mapped_values.count(value) == 0)
            kept_values.insert(value);
    }
    kept_values.insert(projected_value);
    find_and_apply_new_fact_ordering(var, kept_values, projected_value);

    // Set combined fact_name.
    stringstream name;
    string sep = "";
    for (int value : mapped_values) {
        name << sep << fact_names[var][value];
        sep = " OR ";
    }
    fact_names[var][projected_value] = name.str();
}

LandmarkTask LandmarkTask::get_original_task() {
    LandmarkTask task(g_variable_domain, g_fact_names, g_initial_state_data, g_goal);
    return task;
}

void LandmarkTask::install() {
    g_goal = goals;
    g_variable_domain = variable_domain;
    g_fact_names = fact_names;
    Values::initialize_static_members(variable_domain);
    g_initial_state_data = initial_state_data;
}

bool LandmarkTask::translate_state(const GlobalState &state, int *translated) const {
    // TODO: Loop only over changed values.
    int num_vars = variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        int value = task_index[var][state[var]];
        // TODO: Remove this case and return void.
        if (value == UNDEFINED) {
            return false;
        } else {
            translated[var] = value;
        }
    }
    return true;
}

double LandmarkTask::get_state_space_fraction(const LandmarkTask &global_task) const {
    double fraction = 1.0;
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        assert(variable_domain[var] <= global_task.get_variable_domain()[var]);
        fraction *= (variable_domain[var] / static_cast<double>(global_task.get_variable_domain()[var]));
    }
    return fraction;
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
    for (size_t i = 0; i < goals.size(); ++i) {
        cout << sep << goals[i].first << "=" << orig_index[goals[i].first][goals[i].second]
             << ":" << fact_names[goals[i].first][goals[i].second];
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
    return parent->get_fact_name(var, get_orig_value(var, value));
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
