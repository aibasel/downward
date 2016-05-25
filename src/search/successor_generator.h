#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include "task_proxy.h"

#include <list>
#include <memory>
#include <vector>

class AbstractTask;
class GeneratorBase;
class GlobalOperator;
class GlobalState;
class OperatorProxy;
class State;

class SuccessorGenerator {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;

    std::unique_ptr<GeneratorBase> root;

    typedef std::vector<FactProxy> Condition;
    GeneratorBase *construct_recursive(
        int switch_var_id, std::list<OperatorProxy> &operator_queue);

    std::vector<Condition> conditions;
    std::vector<Condition::const_iterator> next_condition_by_op;

    SuccessorGenerator(const SuccessorGenerator &) = delete;
public:
    SuccessorGenerator(const std::shared_ptr<AbstractTask> task);
    ~SuccessorGenerator();

    void generate_applicable_ops(
        const State &state, std::vector<OperatorProxy> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    void generate_applicable_ops(
        const GlobalState &state, std::vector<const GlobalOperator *> &applicable_ops) const;
};

#endif
