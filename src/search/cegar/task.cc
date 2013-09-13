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

Task Task::get_original_task() {
    Task task;
    //task.operators = g_operators;
    task.goal = g_goal;
    task.variable_domain = g_variable_domain;
    return task;
}

}
