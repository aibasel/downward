#include "task.h"

using namespace std;

namespace cegar_heuristic {


Task::Task() {
}

void Task::install() {
    // Do not change g_operators.
    g_goal = goal;
    g_variable_domain = variable_domain;
}

void Task::release_memory() {
    vector<Operator>().swap(operators);
}

Task Task::get_original_task() {
    Task task;
    //task.operators = g_operators;
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
