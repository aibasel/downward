#ifndef TASKS_MODIFIED_GOALS_TASK_H
#define TASKS_MODIFIED_GOALS_TASK_H

#include "../delegating_task.h"

#include <utility>
#include <vector>

namespace Tasks {
class ModifiedGoalsTask : public DelegatingTask {
    const std::vector<std::pair<int, int>> goals;

public:
    ModifiedGoalsTask(
        const std::shared_ptr<AbstractTask> parent,
        std::vector<std::pair<int, int>> &&goals);

    virtual int get_num_goals() const override;
    virtual std::pair<int, int> get_goal_fact(int index) const override;
};
}

#endif
