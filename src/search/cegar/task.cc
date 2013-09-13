#include "task.h"

using namespace std;

namespace cegar_heuristic {


Task::Task() {
}

Task Task::save_original_task() {
    Task task;
    task.goal = g_goal;
    task.operators = g_operators;
    return task;
}

}
