#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include "task_proxy.h"

#include <iostream>
#include <list>
#include <memory>
#include <vector>

class AbstractTask;
class GeneratorBase;
class Operator;
class OperatorProxy;
class State;
class Variable;

class SuccessorGenerator {
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;

    GeneratorBase *root;

    typedef std::vector<std::pair<Variable *, int> > Condition;
    GeneratorBase *construct_recursive(int switchVarNo, std::list<int> &ops);

    std::vector<Condition> conditions;
    std::vector<Condition::const_iterator> next_condition_by_op;
    std::vector<Variable *> varOrder;

    SuccessorGenerator(const SuccessorGenerator &) = delete;
public:
    SuccessorGenerator();
    SuccessorGenerator(std::shared_ptr<AbstractTask> task);
    ~SuccessorGenerator() = default;

    void dump() const;
    void generate_cpp_input(ofstream &outfile) const;

    void generate_applicable_ops(const State &state,
                                 std::vector<OperatorProxy> &applicable_ops);
};

#endif
