#include "task.h"

#include "values.h"
#include "../state_registry.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <set>

using namespace std;

namespace cegar {
Task::Task(vector<int> domain, vector<vector<string> > names, vector<GlobalOperator> ops,
           vector<int> initial_state_data_, vector<Fact> goal_facts)
    : initial_state_data(initial_state_data_),
      goal(goal_facts),
      variable_domain(domain),
      unreachable_facts(domain.size()),
      fact_names(names),
      operators(ops),
      original_operator_numbers(ops.size()),
      orig_index(domain.size()),
      task_index(domain.size()),
      additive_heuristic(0) {
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        orig_index[var].resize(variable_domain[var]);
        task_index[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            orig_index[var][value] = value;
            task_index[var][value] = value;
        }
    }
    for (size_t i = 0; i < operators.size(); ++i)
        original_operator_numbers[i] = i;
}

bool operator_applicable(const GlobalOperator &op, const FactSet &reached) {
    for (size_t i = 0; i < op.get_preconditions().size(); i++) {
        const GlobalCondition &precondition = op.get_preconditions()[i];
        if (reached.count(Fact(precondition.var, precondition.val)) == 0)
            return false;
    }
    return true;
}

void Task::compute_possibly_before_facts(const Fact &last_fact, FactSet *reached) {
    // Add facts from initial state.
    for (size_t var = 0; var < variable_domain.size(); ++var)
        reached->insert(Fact(var, initial_state_data[var]));

    // Until no more facts can be added:
    size_t last_num_reached = 0;
    while (last_num_reached != reached->size()) {
        last_num_reached = reached->size();
        for (size_t i = 0; i < operators.size(); ++i) {
            GlobalOperator &op = operators[i];
            // Ignore operators that achieve last_fact.
            if (get_eff(op, last_fact.first) == last_fact.second)
                continue;
            // Add all facts that are achieved by an applicable operator.
            if (operator_applicable(op, *reached)) {
                for (size_t i = 0; i < op.get_effects().size(); i++) {
                    const GlobalEffect &effect = op.get_effects()[i];
                    reached->insert(Fact(effect.var, effect.val));
                }
            }
        }
    }
}

void Task::remove_unmarked_operators() {
    assert(operators.size() == original_operator_numbers.size());
    vector<int> new_original_operator_numbers;
    for (size_t i = 0; i < original_operator_numbers.size(); ++i) {
        if (operators[i].is_marked())
            new_original_operator_numbers.push_back(original_operator_numbers[i]);
    }
    operators.erase(remove_if(operators.begin(), operators.end(), is_not_marked), operators.end());
    original_operator_numbers = new_original_operator_numbers;
    assert(operators.size() == original_operator_numbers.size());
}

void Task::remove_inapplicable_operators(const FactSet reached_facts) {
    for (size_t i = 0; i < operators.size(); ++i) {
        GlobalOperator &op = operators[i];
        op.unmark();
        if (operator_applicable(op, reached_facts))
            op.mark();
    }
    remove_unmarked_operators();
}

void Task::keep_single_effect(const Fact &last_fact) {
    for (size_t i = 0; i < operators.size(); ++i) {
        GlobalOperator &op = operators[i];
        // If op achieves last_fact set eff(op) = {last_fact}.
        if (get_eff(op, last_fact.first) == last_fact.second)
            op.keep_single_effect(last_fact.first, last_fact.second);
    }
}

void Task::set_goal(const Fact &fact) {
    additive_heuristic = 0;
    goal.clear();
    goal.push_back(fact);
}

void Task::adapt_operator_costs(const vector<int> &remaining_costs) {
    if (operators.size() != original_operator_numbers.size())
        ABORT("Updating original_operator_numbers not implemented.");
    for (size_t i = 0; i < operators.size(); ++i) {
        operators[i].set_cost(remaining_costs[original_operator_numbers[i]]);
    }
}

void Task::adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) const {
    if (DEBUG)
        cout << "Remaining: " << to_string(remaining_costs) << endl;
    if (DEBUG)
        cout << "Needed:    " << to_string(needed_costs) << endl;
    assert(operators.size() == original_operator_numbers.size());
    for (size_t i = 0; i < operators.size(); ++i) {
        int op_number = original_operator_numbers[i];
        assert(in_bounds(op_number, remaining_costs));
        assert(remaining_costs[op_number] >= 0);
        assert(needed_costs[i] <= remaining_costs[op_number]);
        remaining_costs[op_number] -= needed_costs[i];
        assert(remaining_costs[op_number] >= 0);
    }
    if (DEBUG)
        cout << "Remaining: " << to_string(remaining_costs) << endl;
}

void Task::move_fact(int var, int before, int after) {
    if (DEBUG)
        cout << "Move fact " << var << ": " << before << " -> " << after << endl;
    assert(in_bounds(before, task_index[var]));
    if (after == UNDEFINED) {
        assert(initial_state_data[var] != before);
        for (size_t i = 0; i < goal.size(); ++i) {
            assert(goal[i].first != var || goal[i].second != before);
        }
        task_index[var][orig_index[var][before]] = UNDEFINED;
        return;
    }
    assert(in_bounds(after, task_index[var]));
    for (size_t i = 0; i < operators.size(); ++i)
        operators[i].rename_fact(var, before, after);
    // We never move a fact with more than one original index. If we did, we
    // would have to use a vector here.
    orig_index[var][after] = orig_index[var][before];
    assert(orig_index[var][before] == before);
    task_index[var][orig_index[var][before]] = after;
    fact_names[var][after] = fact_names[var][before];
    if (initial_state_data[var] == before)
        initial_state_data[var] = after;
    for (size_t i = 0; i < goal.size(); ++i) {
        if (var == goal[i].first && before == goal[i].second)
            goal[i].second = after;
    }
    unordered_set<int>::iterator it = unreachable_facts[var].find(before);
    if (it != unreachable_facts[var].end()) {
        unreachable_facts[var].erase(it);
        unreachable_facts[var].insert(after);
    }
}

void Task::update_facts(int var, int num_values, const vector<int> &new_task_index) {
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

void Task::find_and_apply_new_fact_ordering(int var, set<int> &unordered_values, int value_for_rest) {
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

void Task::save_unreachable_facts(const FactSet &reached_facts) {
    assert(!reached_facts.empty());
    int num_vars = variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        int num_values = variable_domain[var];
        assert(static_cast<int>(task_index[var].size()) == num_values);
        assert(unreachable_facts[var].empty());
        set<int> unordered_values;
        for (int value = 0; value < num_values; ++value) {
            if (reached_facts.count(Fact(var, value)) == 1) {
                unordered_values.insert(value);
            } else {
                if (DEBUG)
                    cout << "Remove fact " << Fact(var, value) << endl;
                unreachable_facts[var].insert(value);
            }
        }
        //find_and_apply_new_fact_ordering(var, unordered_values, UNDEFINED);
    }
}

void Task::combine_facts(int var, unordered_set<int> &values) {
    assert(values.size() >= 2);
    set<int> mapped_values;
    for (unordered_set<int>::iterator it = values.begin(); it != values.end(); ++it) {
        assert(task_index[var][*it] != UNDEFINED);
        mapped_values.insert(task_index[var][*it]);
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
    for (set<int>::iterator it = mapped_values.begin(); it != mapped_values.end(); ++it) {
        name << sep << fact_names[var][*it];
        sep = " OR ";
    }
    fact_names[var][projected_value] = name.str();
}

void Task::setup_hadd() {
    cout << "Start computing h^add values [t=" << g_timer << "] for ";
    dump_name();
    Options opts;
    opts.set<int>("cost_type", 0);
    additive_heuristic = new AdditiveHeuristic(opts);
    g_initial_state_data = initial_state_data;
    StateRegistry *registry = new StateRegistry();
    const GlobalState &initial_state = registry->get_initial_state();
    int num_vars = variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        assert(initial_state[var] == initial_state_data[var]);
    }
    additive_heuristic->evaluate(initial_state);
    if (false) {
        cout << "h^add values for all facts:" << endl;
        for (int var = 0; var < num_vars; ++var) {
            for (int value = 0; value < variable_domain[var]; ++value) {
                cout << "  " << var << "=" << value << " " << fact_names[var][value]
                     << " cost:" << additive_heuristic->get_cost(var, value) << endl;
            }
        }
        cout << endl;
    }
    cout << "Done computing h^add values [t=" << g_timer << "]" << endl;
    delete registry;
}

int Task::get_hadd_value(int var, int value) const {
    assert(additive_heuristic);
    assert(in_bounds(var, variable_domain));
    assert(value < variable_domain[var]);
    return additive_heuristic->get_cost(var, value);
}

Task Task::get_original_task() {
    Task task(g_variable_domain, g_fact_names, g_operators, g_initial_state_data, g_goal);
    task.setup_hadd();
    return task;
}

void Task::install() {
    g_goal = goal;
    g_variable_domain = variable_domain;
    g_fact_names = fact_names;
    g_operators = operators;
    Values::initialize_static_members(variable_domain);
    setup_hadd();
}

void Task::release_memory() {
    vector<GlobalOperator>().swap(operators);
    delete additive_heuristic;
    additive_heuristic = 0;
}

bool Task::translate_state(const GlobalState &state, int *translated) const {
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

double Task::get_state_space_fraction(const Task &global_task) const {
    double fraction = 1.0;
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        assert(variable_domain[var] <= global_task.get_variable_domain()[var]);
        fraction *= (variable_domain[var] / static_cast<double>(global_task.get_variable_domain()[var]));
    }
    return fraction;
}

void Task::dump_facts() const {
    for (size_t var = 0; var < variable_domain.size(); ++var) {
        for (int value = 0; value < variable_domain[var]; ++value)
            cout << "    " << var << "=" << value << ": " << fact_names[var][value] << endl;
    }
}

void Task::dump_name() const {
    cout << "Task ";
    string sep = "";
    for (size_t i = 0; i < goal.size(); ++i) {
        cout << sep << goal[i].first << "=" << orig_index[goal[i].first][goal[i].second]
             << ":" << fact_names[goal[i].first][goal[i].second];
        sep = " ";
    }
    cout << endl;
}

void Task::dump() const {
    dump_name();
    int num_facts = 0;
    for (size_t var = 0; var < variable_domain.size(); ++var)
        num_facts += variable_domain[var];
    cout << "  Facts: " << num_facts << endl;
    if (DEBUG)
        dump_facts();
    cout << "  GlobalOperators: " << operators.size() << endl;
    int total_cost = 0;
    for (size_t i = 0; i < operators.size(); ++i) {
        total_cost += operators[i].get_cost();
    }
    assert(total_cost >= 0);
    cout << "  Total cost: " << total_cost << endl;
    if (DEBUG) {
        for (size_t i = 0; i < operators.size(); ++i) {
            cout << "    c=" << operators[i].get_cost() << " ";
            operators[i].dump();
        }
    }
    cout << "  " << "Variable domain sizes: " << to_string(variable_domain) << endl;
    if (DEBUG) {
        cout << "  " << "Fact mapping:" << endl;
        for (size_t var = 0; var < task_index.size(); ++var)
            cout << "    " << var << ": " << to_string(task_index[var]) << endl;
    }
}
}
