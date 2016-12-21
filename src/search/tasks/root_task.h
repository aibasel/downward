#ifndef TASKS_ROOT_TASK_H
#define TASKS_ROOT_TASK_H

#include "explicit_task.h"

namespace tasks {
class RootTask : public ExplicitTask {
public:
    RootTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<ExplicitVariable> &&variables,
        std::vector<std::vector<std::set<FactPair>>> &&mutexes,
        std::vector<ExplicitOperator> &&operators,
        std::vector<ExplicitOperator> &&axioms,
        std::vector<int> &&initial_state_values,
        std::vector<FactPair> &&goals);

    virtual const GlobalOperator *get_global_operator(int index, bool is_axiom) const override;
    virtual void convert_state_values_from_parent(std::vector<int> &values) const override;
};

/*
  Eventually, this should parse the task from a stream.
  this is currently done by the methods read_* in globals.
  We have to keep them as long as there is still code that
  accesses the global variables that store the task. For
  now, we let those methods do the parsing and access the
  parsed task here.
*/
std::shared_ptr<RootTask> create_root_task(
    const std::vector<std::string> &variable_name,
    const std::vector<int> &variable_domain,
    const std::vector<std::vector<std::string>> &fact_names,
    const std::vector<int> &axiom_layers,
    const std::vector<int> &default_axiom_values,
    const std::vector<std::vector<std::set<FactPair>>> &inconsistent_facts,
    const std::vector<int> &initial_state_data,
    const std::vector<std::pair<int, int>> &goal,
    const std::vector<GlobalOperator> &operators,
    const std::vector<GlobalOperator> &axioms);
}

#endif
