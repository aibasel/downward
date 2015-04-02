#ifndef CEGAR_MODIFIED_GOALS_TASK_H
#define CEGAR_MODIFIED_GOALS_TASK_H

#include "../delegating_task.h"

#include <utility>
#include <vector>

namespace cegar {
class ModifiedGoalsTask : public DelegatingTask {
private:
    const std::vector<std::pair<int, int> > goals;

public:
    ModifiedGoalsTask(std::shared_ptr<AbstractTask> parent, std::vector<std::pair<int, int> > &goals);

    virtual int get_num_goals() const override;
    virtual std::pair<int, int> get_goal_fact(int index) const override;
};
}

#endif
