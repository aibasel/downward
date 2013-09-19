#include "task.h"

#include <set>

using namespace std;

namespace cegar_heuristic {

Task::Task()
    : goal(),
      variable_domain(g_variable_domain),
      fact_names(g_fact_names),
      operators(),
      original_operator_numbers(),
      fact_numbers(),
      fact_mapping(g_variable_domain.size()) {
    for (int var = 0; var < variable_domain.size(); ++var) {
        fact_mapping[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            fact_mapping[var][value] = value;
        }
    }
}

void Task::set_goal(vector<Fact> facts) {
    // Pass by value to get a copy.
    goal = facts;
    compute_facts_and_operators();
}

bool operator_applicable(const Operator &op, const unordered_set<int> &reached) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail &prevail = op.get_prevail()[i];
        if (reached.count(get_fact_number(prevail.var, prevail.prev)) == 0)
            return false;
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.pre != UNDEFINED && reached.count(get_fact_number(pre_post.var, pre_post.pre)) == 0)
            return false;
    }
    return true;
}

void Task::compute_possibly_before_facts(const Fact &last_fact) {
    unordered_set<int> *reached = &fact_numbers;
    // Add facts from initial state.
    for (int var = 0; var < g_variable_domain.size(); ++var)
        reached->insert(get_fact_number(var, (*g_initial_state)[var]));

    // Until no more facts can be added:
    int last_num_reached = 0;
    while (last_num_reached != reached->size()) {
        last_num_reached = reached->size();
        for (int i = 0; i < g_operators.size(); ++i) {
            Operator &op = g_operators[i];
            // Ignore operators that achieve last_fact.
            if (get_eff(op, last_fact.first) == last_fact.second)
                continue;
            // Add all facts that are achieved by an applicable operator.
            if (operator_applicable(op, *reached)) {
                for (int i = 0; i < op.get_pre_post().size(); i++) {
                    const PrePost &pre_post = op.get_pre_post()[i];
                    reached->insert(get_fact_number(pre_post.var, pre_post.post));
                }
            }
        }
    }
}

void Task::compute_facts_and_operators() {
    if (goal.size() > 1) {
        operators = g_operators;
        for (int i = 0; i < operators.size(); ++i)
            original_operator_numbers.push_back(i);
        for (int fact_number = 0; fact_number < g_num_facts; ++fact_number) {
            // TODO: Use hint.
            fact_numbers.insert(fact_number);
        }
        return;
    }

    assert(goal.size() == 1);
    Fact &last_fact = goal[0];
    compute_possibly_before_facts(last_fact);

    for (int i = 0; i < g_operators.size(); ++i) {
        // Only keep operators with all preconditions in reachable set of facts.
        if (operator_applicable(g_operators[i], fact_numbers)) {
            operators.push_back(g_operators[i]);
            original_operator_numbers.push_back(i);
        }
    }
    for (int i = 0; i < operators.size(); ++i) {
        Operator &op = operators[i];
        // If op achieves last_fact set eff(op) = {last_fact}.
        if (get_eff(op, last_fact.first) == last_fact.second)
            op.keep_single_effect(last_fact.first);
    }
    // Add last_fact to reachable facts.
    fact_numbers.insert(get_fact_number(last_fact.first, last_fact.second));
    remove_unreachable_facts();
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
        assert(!operators[i].is_marked());
    assert(goal.size() == 1);
    Fact &last_fact = goal[0];
    mark_relevant_operators(last_fact);
    if (DEBUG) {
        int num_relevant_ops = 0;
        for (int i = 0; i < g_operators.size(); ++i) {
            if (g_operators[i].is_marked())
                ++num_relevant_ops;
        }
        cout << "Relevant operators: " << num_relevant_ops << "/" << g_operators.size() << endl;
    }
    remove_if(operators.begin(), operators.end(), is_marked);
}

void Task::adapt_operator_costs(const vector<int> &remaining_costs) {
    assert(operators.size() == original_operator_numbers.size());
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

bool Task::state_is_reachable(const State &state) const {
    assert(g_variable_domain.size() == g_fact_borders.size());
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        if (fact_numbers.count(get_fact_number(var, state[var])) == 0)
            return false;
    }
    return true;
}

void Task::project_state(State &state) const {
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        state[var] = fact_mapping[var][state[var]];
    }
}

void Task::install() {
    // Do not change g_operators.
    g_goal = goal;
    g_variable_domain = variable_domain;
}

void Task::rename_fact(int var, int before, int after) {
    for (int i = 0; i < operators.size(); ++i)
        operators[i].rename_fact(var, before, after);
    fact_mapping[var][before] = after;
}

void Task::remove_fact(const Fact &fact) {
    assert(!variable_domain.empty());
    assert(variable_domain[fact.first] >= 2);
    --variable_domain[fact.first];
    fact_names[fact.first].erase(fact_names[fact.first].begin() + fact.second);
}

void Task::shrink_domain(int var, int shrink_by) {
    assert(!variable_domain.empty());
    assert(variable_domain[var] > shrink_by);
    variable_domain[var] -= shrink_by;
}

void Task::remove_unreachable_facts() {
    assert(!fact_numbers.empty());
    for (int fact_number = 0; fact_number < g_num_facts; ++fact_number) {
        if (fact_numbers.count(fact_number) == 0) {
            Fact fact = get_fact(fact_number);
            // Rename all values of this variable after the unreachable fact.
            for (int value = fact.second + 1; value < variable_domain[fact.first]; ++value) {
                rename_fact(fact.first, value, value - 1);
            }
            remove_fact(fact);
        }
    }
}

void Task::combine_facts(int var, const vector<int> &values) {
    cout << var << " " << to_string(values) << endl;
    assert(values.size() >= 2);
    // Map all values to the first value.
    for (int i = 1; i < values.size(); ++i) {
        rename_fact(var, values[i], values[0]);
    }
    shrink_domain(var, values.size() - 1);
}

void Task::release_memory() {
    vector<Operator>().swap(operators);
}

Task Task::get_original_task() {
    Task task;
    task.set_goal(g_goal);
    return task;
}

void Task::dump_facts() const {
    set<int> ordered_fact_numbers(fact_numbers.begin(), fact_numbers.end());
    for (auto it = ordered_fact_numbers.begin(); it != ordered_fact_numbers.end(); ++it) {
        Fact fact = get_fact(*it);
        cout << "    " << fact.first << "=" << fact.second
             << " (" << *it << ")" << ": "
             << fact_names[fact.first][fact.second] << endl;
    }
}

void Task::dump() const {
    cout << "Task ";
    for (int j = 0; j < goal.size(); ++j)
        cout << goal[j].first << "=" << goal[j].second << ":"
             << fact_names[goal[j].first][goal[j].second] << " ";
    cout << endl;
    cout << "  Facts: " << fact_numbers.size() << "/" << g_num_facts << endl;
    if (DEBUG)
        dump_facts();
    cout << "  Operators: " << operators.size() << "/" << g_operators.size() << endl;
    if (DEBUG) {
        for (int i = 0; i < operators.size(); ++i) {
            cout << "    ";
            operators[i].dump();
        }
    }
    cout << "  Variable domain sizes: " << to_string(variable_domain) << endl;
    if (DEBUG) {
        cout << "  Fact mapping:" << endl;
        for (int var = 0; var < variable_domain.size(); ++var) {
            for (int value = 0; value < variable_domain[var]; ++value)
                cout << "    " << var << ": " << value << " -> " << fact_mapping[var][value] << endl;
        }
    }
    cout << endl;
}

}
