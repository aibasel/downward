#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include "task_proxy.h"

#include <iostream>
#include <list>
#include <memory>
#include <vector>

class AbstractTask;
class GeneratorBase;
class OperatorProxy;
class State;
class Variable;

class SuccessorGenerator {
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;

    GeneratorBase *root;

    typedef std::vector<std::pair<VariableProxy, int> > Condition;
    GeneratorBase *construct_recursive(
        int switch_var_id, list<OperatorProxy> &operator_queue);

    std::vector<Condition> conditions;
    std::vector<Condition::const_iterator> next_condition_by_op;

    SuccessorGenerator(const SuccessorGenerator &) = delete;
public:
    SuccessorGenerator();
    SuccessorGenerator(std::shared_ptr<AbstractTask> task);
    ~SuccessorGenerator() = default;

    void dump() const;
    void generate_cpp_input(std::ofstream &outfile) const;

    void generate_applicable_ops(const State &state,
                                 std::vector<OperatorProxy> &applicable_ops);
};

#endif
