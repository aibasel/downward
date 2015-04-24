#ifndef CEGAR_DOMAIN_ABSTRACTED_TASK_BUILDER_H
#define CEGAR_DOMAIN_ABSTRACTED_TASK_BUILDER_H

#include "domain_abstracted_task.h"

#include <memory>
#include <string>
#include <vector>

class AbstractTask;

namespace cegar {
class DomainAbstractedTaskBuilder {
private:
    std::shared_ptr<AbstractTask> parent;
    std::vector<int> domain_size;
    std::vector<int> initial_state_values;
    std::vector<Fact> goals;
    std::vector<std::vector<std::string> > fact_names;
    std::vector<std::vector<int> > value_map;

    void initialize(const std::shared_ptr<AbstractTask> parent);
    void move_fact(int var, int before, int after);
    void combine_values(int var, const ValueGroups &groups);
    std::string get_combined_fact_name(int var, const ValueGroup &values) const;

public:
    std::shared_ptr<AbstractTask> get_task(
        const std::shared_ptr<AbstractTask> parent,
        const VarToGroups &value_groups);
};
}

#endif
