#include "modified_goals_task.h"

using namespace std;

namespace extra_tasks {
ModifiedGoalsTask::ModifiedGoalsTask(
    const shared_ptr<AbstractTask> &parent,
    vector<FactPair> &&goals)
    : DelegatingTask(parent),
      goals(move(goals)) {
}

int ModifiedGoalsTask::get_num_goals() const {
    return goals.size();
}

FactPair ModifiedGoalsTask::get_goal_fact(int index) const {
    return goals[index];
}
}
