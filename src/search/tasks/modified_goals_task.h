#ifndef TASKS_MODIFIED_GOALS_TASK_H
#define TASKS_MODIFIED_GOALS_TASK_H

#include "../delegating_task.h"

#include <utility>
#include <vector>

namespace ExtraTasks {
class ModifiedGoalsTask : public DelegatingTask {
    const std::vector<Fact> goals;

public:
    ModifiedGoalsTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<Fact> &&goals);
    ~ModifiedGoalsTask() = default;

    virtual int get_num_goals() const override;
    virtual Fact get_goal_fact(int index) const override;
};
}

#endif
