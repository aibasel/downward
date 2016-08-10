#ifndef TASKS_MODIFIED_GOALS_TASK_H
#define TASKS_MODIFIED_GOALS_TASK_H

#include "delegating_task.h"

#include <vector>

namespace extra_tasks {
class ModifiedGoalsTask : public tasks::DelegatingTask {
    const std::vector<FactPair> goals;

public:
    ModifiedGoalsTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<FactPair> &&goals);
    ~ModifiedGoalsTask() = default;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;
};
}

#endif
