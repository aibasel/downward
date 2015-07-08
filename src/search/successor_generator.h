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

    typedef std::vector<FactProxy> Condition;
    GeneratorBase *construct_recursive(
        int switch_var_id, std::list<OperatorProxy> &operator_queue);

    std::vector<Condition> conditions;
    std::vector<Condition::const_iterator> next_condition_by_op;

    SuccessorGenerator(const SuccessorGenerator &) = delete;
public:
    SuccessorGenerator(std::shared_ptr<AbstractTask> task);
    SuccessorGenerator(SuccessorGenerator &&other) noexcept;
    SuccessorGenerator& operator=(SuccessorGenerator &&other) noexcept;
    ~SuccessorGenerator();

    void dump() const;
    void generate_cpp_input(std::ofstream &outfile) const;

    void generate_applicable_ops(const State &state,
                                 std::vector<OperatorProxy> &applicable_ops);
};

#endif
