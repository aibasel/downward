#include "task.h"

using namespace std;

namespace cegar_heuristic {


Task::Task() {
}

void Task::install() const {
    g_goal = goal;
}

Task Task::save_original_task() {
    return Task();
}

}
