#include "task.h"

using namespace std;

namespace cegar_heuristic {

Task::Task()
    : fact_mapping(g_variable_domain.size()),
      goal(),
      variable_domain(),
      operators(),
      original_operator_numbers(),
      fact_numbers() {
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        fact_mapping[var].resize(g_variable_domain[var]);
        for (int value = 0; value < g_variable_domain[var]; ++value)
            fact_mapping[var][value] = value;
    }
}

void Task::set_goal(vector<Fact> facts) {
    // Pass by value to get a copy.
    goal = facts;
}

void Task::install() {
    // Do not change g_operators.
    g_goal = goal;
    g_variable_domain = variable_domain;
}

void Task::rename_fact(int var, int before, int after) {
    for (int i = 0; i < operators.size(); ++i)
        operators[i].rename_fact(var, before, after);
}

void Task::remove_fact(const Fact &fact) {
    assert(!variable_domain.empty());
    assert(variable_domain[fact.first] >= 2);
    --variable_domain[fact.first];
}

void Task::remove_unreachable_facts() {
    assert(!fact_numbers.empty());
    for (int fact_number = 0; fact_number < g_num_facts; ++fact_number) {
        if (fact_numbers.count(fact_number) == 0)
            remove_fact(get_fact(fact_number));
    }
}

void Task::combine_facts(int var, const vector<int> &values) {
    cout << var << " " << to_string(values) << endl;
    assert(values.size() >= 2);
    // Map all values to the first value.
    for (int i = 1; i < values.size(); ++i) {
        rename_fact(var, values[i], values[0]);
        remove_fact(Fact(var, values[i]));
        fact_mapping[var][values[i]] = values[0];
    }
}

void Task::release_memory() {
    vector<Operator>().swap(operators);
}

Task Task::get_original_task() {
    Task task;
    task.goal = g_goal;
    task.variable_domain = g_variable_domain;
    for (int fact_number = 0; fact_number < g_num_facts; ++fact_number) {
        // TODO: Use hint.
        task.fact_numbers.insert(fact_number);
    }
    assert(task.fact_numbers.size() == g_num_facts);
    return task;
}

void Task::dump_facts() const {
    for (auto it = fact_numbers.begin(); it != fact_numbers.end(); ++it) {
        Fact fact = get_fact(*it);
        cout << "PB fact " << *it << ": " << g_fact_names[fact.first][fact.second] << endl;
    }
}

}
