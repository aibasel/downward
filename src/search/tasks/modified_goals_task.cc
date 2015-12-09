#include "modified_goals_task.h"

using namespace std;

namespace tasks {
ModifiedGoalsTask::ModifiedGoalsTask(
    const std::shared_ptr<AbstractTask> parent,
    std::vector<std::pair<int, int>> &&goals)
    : DelegatingTask(parent),
      goals(move(goals)) {
}

int ModifiedGoalsTask::get_num_goals() const {
    return goals.size();
}

pair<int, int> ModifiedGoalsTask::get_goal_fact(int index) const {
    return goals[index];
}
}
