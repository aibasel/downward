#include "task.h"

using namespace std;

namespace cegar_heuristic {


Task::Task() {
}

void Task::install() const {
    g_goal = goal;
    g_operators = operators;
}

Task Task::save_original_task() {
    Task task;
    task.goal = g_goal;
    task.operators = g_operators;
    return task;
}

}
