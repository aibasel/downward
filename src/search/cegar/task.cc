#include "task.h"

#include "values.h"
#include "../timer.h"

#include <set>

using namespace std;

namespace cegar_heuristic {

Task::Task(vector<Fact> goal_facts, bool adapt)
    : initial_state(*g_initial_state),
      goal(goal_facts),
      variable_domain(g_variable_domain),
      fact_names(g_fact_names),
      operators(),
      original_operator_numbers(),
      orig_index(g_variable_domain.size()),
      task_index(g_variable_domain.size()),
      additive_heuristic(0),
      adapt_task(adapt),
      is_original_task(false) {
    for (int var = 0; var < variable_domain.size(); ++var) {
        orig_index[var].resize(variable_domain[var]);
        task_index[var].resize(variable_domain[var]);
        for (int value = 0; value < variable_domain[var]; ++value) {
            orig_index[var][value] = value;
            task_index[var][value] = value;
        }
    }
    compute_facts_and_operators();
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
    for (int var = 0; var < g_variable_domain.size(); ++var)
        reached->insert(Fact(var, (*g_initial_state)[var]));

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
                    reached->insert(Fact(pre_post.var, pre_post.post));
                }
            }
        }
    }
}

void Task::compute_facts_and_operators() {
    if (!adapt_task) {
        operators = g_operators;
        for (int i = 0; i < operators.size(); ++i)
            original_operator_numbers.push_back(i);
        return;
    }

    assert(goal.size() == 1);
    Fact &last_fact = goal[0];
    FactSet reached_facts;
    compute_possibly_before_facts(last_fact, &reached_facts);

    for (int i = 0; i < g_operators.size(); ++i) {
        // Only keep operators with all preconditions in reachable set of facts.
        if (operator_applicable(g_operators[i], reached_facts)) {
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
        assert(!operators[i].is_marked());
    assert(goal.size() == 1);
    Fact &last_fact = goal[0];
    mark_relevant_operators(last_fact);
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
    for (int var = 0; var < g_variable_domain.size(); ++var) {
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
    variable_domain[var] = num_values;
    assert(orig_index[var].size() == variable_domain[var]);
    assert(fact_names[var].size() == variable_domain[var]);

    if (DEBUG && num_values < g_variable_domain[var]) {
        cout << "Task index " << var << ": " << to_string(task_index[var]) << endl;
        cout << "Orig index " << var << ": " << to_string(orig_index[var]) << endl;
    }
}

void Task::remove_unreachable_facts(const FactSet &reached_facts) {
    assert(!reached_facts.empty());
    for (int var = 0; var < variable_domain.size(); ++var) {
        vector<int> new_task_index(variable_domain[var], -2);
        int num_values = 0;
        for (int value = 0; value < variable_domain[var]; ++value) {
            if (reached_facts.count(Fact(var, value)) == 0) {
                if (DEBUG)
                    cout << "Remove fact " << Fact(var, value) << endl;
                new_task_index[value] = UNDEFINED;
            } else {
                new_task_index[value] = num_values++;
            }
        }
        update_facts(var, num_values, new_task_index);
    }
}

void Task::combine_facts(int var, unordered_set<int> &values) {
    assert(values.size() >= 2);
    set<int> ordered_values;
    for (auto it = values.begin(); it != values.end(); ++it) {
        ordered_values.insert(task_index[var][*it]);
    }
    assert(is_sorted(ordered_values.begin(), ordered_values.end()));
    if (DEBUG)
        cout << "Combine " << var << ": " << " mapped "
             << to_string(ordered_values) << endl;
    int projected_value = *ordered_values.begin();
    vector<int> new_task_index(variable_domain[var], -2);
    int num_values = 0;
    bool seen_projected_value = false;
    for (int value = 0; value < variable_domain[var]; ++value) {
        if (ordered_values.count(value) == 0) {
            new_task_index[value] = num_values++;
        } else {
            new_task_index[value] = projected_value;
            if (!seen_projected_value) {
                ++num_values;
                seen_projected_value = true;
            }
        }
    }
    assert(seen_projected_value);
    assert(num_values + ordered_values.size() - 1 == variable_domain[var]);
    update_facts(var, num_values, new_task_index);

    // Set combined fact_name.
    stringstream name;
    string sep = "";
    for (auto it = ordered_values.begin(); it != ordered_values.end(); ++it) {
        name << sep << fact_names[var][*it];
        sep = " OR ";
    }
    fact_names[var][projected_value] = name.str();
}

void Task::release_memory() {
    vector<Operator>().swap(operators);
}

Task Task::get_original_task() {
    Task task(g_goal, false);
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
        for (int var = 0; var < g_variable_domain.size(); ++var) {
            for (int value = 0; value < g_variable_domain[var]; ++value) {
                cout << "  " << var << "=" << value << " " << g_fact_names[var][value]
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
    for (int j = 0; j < goal.size(); ++j) {
        cout << sep << goal[j] << ":" << fact_names[goal[j].first][goal[j].second];
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
    cout << endl;
}

}
