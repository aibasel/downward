#include "task.h"

#include "values.h"
#include "../timer.h"

#include <set>

using namespace std;

namespace cegar_heuristic {

Task::Task(vector<int> domain, vector<vector<string> > names, vector<Operator> ops,
           State init, vector<Fact> goal_facts)
    : initial_state(init),
      goal(goal_facts),
      variable_domain(domain),
      fact_names(names),
      operators(ops),
      original_operator_numbers(ops.size()),
      orig_index(domain.size()),
      task_index(domain.size()),
      additive_heuristic(0),
      is_original_task(false) {
    for (int var = 0; var < variable_domain.size(); ++var) {
        orig_index[var].resize(variable_domain[var]);
        task_index[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            orig_index[var][value] = value;
            task_index[var][value] = value;
        }
    }
    for (int i = 0; i < operators.size(); ++i)
        original_operator_numbers[i] = i;
}

bool operator_applicable(const Operator &op, const FactSet &reached) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail &prevail = op.get_prevail()[i];
        if (reached.count(Fact(prevail.var, prevail.prev)) == 0)
            return false;
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.pre != UNDEFINED && reached.count(Fact(pre_post.var, pre_post.pre)) == 0)
            return false;
    }
    return true;
}

void Task::compute_possibly_before_facts(const Fact &last_fact, FactSet *reached) {
    // Add facts from initial state.
    for (int var = 0; var < variable_domain.size(); ++var)
        reached->insert(Fact(var, initial_state[var]));

    // Until no more facts can be added:
    int last_num_reached = 0;
    while (last_num_reached != reached->size()) {
        last_num_reached = reached->size();
        for (int i = 0; i < operators.size(); ++i) {
            Operator &op = operators[i];
            // Ignore operators that achieve last_fact.
            if (get_eff(op, last_fact.first) == last_fact.second)
                continue;
            // Add all facts that are achieved by an applicable operator.
            if (operator_applicable(op, *reached)) {
                // TODO: Mark operator as applicable.
                for (int i = 0; i < op.get_pre_post().size(); i++) {
                    const PrePost &pre_post = op.get_pre_post()[i];
                    reached->insert(Fact(pre_post.var, pre_post.post));
                }
            }
        }
    }
}

void Task::remove_unmarked_operators() {
    assert(operators.size() == original_operator_numbers.size());
    vector<int> new_original_operator_numbers;
    for (int i = 0; i < original_operator_numbers.size(); ++i) {
        if (operators[i].is_marked())
            new_original_operator_numbers.push_back(original_operator_numbers[i]);
    }
    auto new_end = remove_if(operators.begin(), operators.end(), is_not_marked);
    operators.erase(new_end, operators.end());
    original_operator_numbers = new_original_operator_numbers;
    assert(operators.size() == original_operator_numbers.size());
}

void Task::remove_inapplicable_operators(const FactSet reached_facts) {
    for (int i = 0; i < operators.size(); ++i) {
        Operator &op = operators[i];
        op.unmark();
        if (operator_applicable(op, reached_facts))
            op.mark();
    }
    remove_unmarked_operators();
}

void Task::compute_facts_and_operators() {
    assert(goal.size() == 1);
    Fact &last_fact = goal[0];
    FactSet reached_facts;
    compute_possibly_before_facts(last_fact, &reached_facts);

    // Only keep operators with all preconditions in reachable set of facts.
    remove_inapplicable_operators(reached_facts);

    for (int i = 0; i < operators.size(); ++i) {
        Operator &op = operators[i];
        // If op achieves last_fact set eff(op) = {last_fact}.
        if (get_eff(op, last_fact.first) == last_fact.second)
            op.keep_single_effect(last_fact.first);
    }
    // Add last_fact to reachable facts.
    reached_facts.insert(last_fact);
    remove_unreachable_facts(reached_facts);
}

void Task::mark_relevant_operators(const Fact &fact) {
    for (int i = 0; i < operators.size(); ++i) {
        Operator &op = operators[i];
        if (op.is_marked())
            continue;
        if (get_eff(op, fact.first) == fact.second) {
            op.mark();
            for (int j = 0; j < op.get_prevail().size(); ++j) {
                const Prevail &prevail = op.get_prevail()[j];
                mark_relevant_operators(Fact(prevail.var, prevail.prev));
            }
            for (int j = 0; j < op.get_pre_post().size(); ++j) {
                const PrePost &pre_post = op.get_pre_post()[j];
                if (pre_post.pre != UNDEFINED)
                    mark_relevant_operators(Fact(pre_post.var, pre_post.pre));
            }
        }
    }
}

void Task::remove_irrelevant_operators() {
    if (goal.size() > 1)
        return;
    for (int i = 0; i < operators.size(); ++i)
        operators[i].unmark();
    assert(goal.size() == 1);
    Fact &last_fact = goal[0];
    mark_relevant_operators(last_fact);
    remove_unmarked_operators();
}

void Task::set_goal(const Fact &fact) {
    goal.clear();
    goal.push_back(fact);
    compute_facts_and_operators();
    is_original_task = false;
}

void Task::adapt_operator_costs(const vector<int> &remaining_costs) {
    if (operators.size() != original_operator_numbers.size()) {
        cout << "Updating original_operator_numbers not implemented." << endl;
        exit(2);
    }
    for (int i = 0; i < operators.size(); ++i)
        operators[i].set_cost(remaining_costs[original_operator_numbers[i]]);
}

void Task::adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) const {
    if (DEBUG)
        cout << "Needed:    " << to_string(needed_costs) << endl;
    assert(operators.size() == original_operator_numbers.size());
    for (int i = 0; i < operators.size(); ++i) {
        int op_number = original_operator_numbers[i];
        assert(op_number >= 0 && op_number < remaining_costs.size());
        remaining_costs[op_number] -= needed_costs[i];
        assert(remaining_costs[op_number] >= 0);
    }
    if (DEBUG)
        cout << "Remaining: " << to_string(remaining_costs) << endl;
}

bool Task::translate_state(State &state) const {
    for (int var = 0; var < variable_domain.size(); ++var) {
        int value = task_index[var][state[var]];
        if (value == UNDEFINED) {
            return false;
        } else {
            state[var] = value;
        }
    }
    return true;
}

void Task::install() {
    if (!is_original_task)
        assert(!additive_heuristic && "h^add can only be calculated for installed tasks");
    assert(g_initial_state);
    *g_initial_state = initial_state;
    g_goal = goal;
    g_variable_domain = variable_domain;
    g_fact_names = fact_names;
    g_operators = operators;
    Values::initialize_static_members();
}

void Task::move_fact(int var, int before, int after) {
    if (DEBUG)
        cout << "Move fact " << var << ": " << before << " -> " << after << endl;
    assert(0 <= before && before < task_index[var].size());
    if (after == UNDEFINED) {
        assert(initial_state[var] != before);
        for (int i = 0; i < goal.size(); ++i) {
            assert(goal[i].first != var || goal[i].second != before);
        }
        task_index[var][orig_index[var][before]] = UNDEFINED;
        return;
    }
    assert(0 <= after && after < task_index[var].size());
    for (int i = 0; i < operators.size(); ++i)
        operators[i].rename_fact(var, before, after);
    // We never move a fact with more than one original indices. If we did, we
    // would have to use a vector here.
    orig_index[var][after] = orig_index[var][before];
    task_index[var][orig_index[var][before]] = after;
    fact_names[var][after] = fact_names[var][before];
    if (initial_state[var] == before)
        initial_state[var] = after;
    for (int i = 0; i < goal.size(); ++i) {
        if (var == goal[i].first && before == goal[i].second)
            goal[i].second = after;
    }
}

void Task::update_facts(int var, int num_values, const vector<int> &new_task_index) {
    assert(num_values >= 1);
    assert(new_task_index.size() == variable_domain[var]);
    for (int before = 0; before < new_task_index.size(); ++before) {
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
    assert(orig_index[var].size() == variable_domain[var]);
    assert(fact_names[var].size() == variable_domain[var]);
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

void Task::remove_unreachable_facts(const FactSet &reached_facts) {
    assert(!reached_facts.empty());
    for (int var = 0; var < variable_domain.size(); ++var) {
        assert(task_index[var].size() == variable_domain[var]);
        vector<int> new_task_index(variable_domain[var], -2);
        set<int> unordered_values;
        for (int value = 0; value < variable_domain[var]; ++value) {
            if (reached_facts.count(Fact(var, value)) == 1) {
                unordered_values.insert(value);
            } else if (DEBUG) {
                cout << "Remove fact " << Fact(var, value) << endl;
            }
        }
        find_and_apply_new_fact_ordering(var, unordered_values, UNDEFINED);
    }
}

void Task::combine_facts(int var, unordered_set<int> &values) {
    assert(values.size() >= 2);
    set<int> mapped_values;
    for (auto it = values.begin(); it != values.end(); ++it) {
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
    for (auto it = mapped_values.begin(); it != mapped_values.end(); ++it) {
        name << sep << fact_names[var][*it];
        sep = " OR ";
    }
    fact_names[var][projected_value] = name.str();
}

void Task::release_memory() {
    vector<Operator>().swap(operators);
}

Task Task::get_original_task() {
    Task task(g_variable_domain, g_fact_names, g_operators, *g_initial_state, g_goal);
    task.is_original_task = true;
    task.setup_hadd();
    return task;
}

void Task::setup_hadd() const {
    assert(!additive_heuristic);
    cout << "Start computing h^add values [t=" << g_timer << "] for ";
    dump_name();
    Options opts;
    opts.set<int>("cost_type", 0);
    opts.set<int>("memory_padding", 75);
    additive_heuristic = new AdditiveHeuristic(opts);
    additive_heuristic->evaluate(initial_state);
    if (false) {
        cout << "h^add values for all facts:" << endl;
        for (int var = 0; var < variable_domain.size(); ++var) {
            for (int value = 0; value < variable_domain[var]; ++value) {
                cout << "  " << var << "=" << value << " " << fact_names[var][value]
                     << " cost:" << additive_heuristic->get_cost(var, value) << endl;
            }
        }
        cout << endl;
    }
    cout << "Done computing h^add values [t=" << g_timer << "]" << endl;
}

int Task::get_hadd_estimate_for_initial_state() const {
    if (!additive_heuristic)
        setup_hadd();
    return additive_heuristic->get_heuristic();
}

int Task::get_hadd_value(int var, int value) const {
    if (!additive_heuristic)
        setup_hadd();
    return additive_heuristic->get_cost(var, value);
}

void Task::dump_facts() const {
    for (int var = 0; var < variable_domain.size(); ++var) {
        for (int value = 0; value < variable_domain[var]; ++value)
            cout << "    " << var << "=" << value << ": " << fact_names[var][value] << endl;
    }
}

void Task::dump_name() const {
    string prefix = is_original_task ? "Original " : "";
    cout << prefix << "Task ";
    string sep = "";
    for (int i = 0; i < goal.size(); ++i) {
        cout << sep << goal[i].first << "=" << orig_index[goal[i].first][goal[i].second]
             << ":" << fact_names[goal[i].first][goal[i].second];
        sep = " ";
    }
    cout << endl;
}

void Task::dump() const {
    string prefix = is_original_task ? "Original " : "";
    dump_name();
    int num_facts = 0;
    for (int var = 0; var < variable_domain.size(); ++var)
        num_facts += variable_domain[var];
    cout << "  " << prefix << "Facts: " << num_facts << "/" << g_num_facts << endl;
    if (DEBUG)
        dump_facts();
    cout << "  " << prefix << "Operators: " << operators.size() << endl;
    if (DEBUG) {
        for (int i = 0; i < operators.size(); ++i) {
            cout << "    ";
            operators[i].dump();
        }
    }
    cout << "  " << prefix << "Variable domain sizes: " << to_string(variable_domain) << endl;
    if (DEBUG) {
        cout << "  " << prefix << "Fact mapping:" << endl;
        for (int var = 0; var < task_index.size(); ++var)
            cout << "    " << var << ": " << to_string(task_index[var]) << endl;
    }
}

}
