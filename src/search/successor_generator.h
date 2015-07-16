#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include "task_proxy.h"

#include <iostream>
#include <memory>
#include <vector>

class AbstractTask;
class OperatorProxy;
class State;

class SuccessorGenerator {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
public:
    SuccessorGenerator(const std::shared_ptr<AbstractTask> task);
    ~SuccessorGenerator() = default;
    void generate_applicable_ops(const State &state,
                                 std::vector<OperatorProxy> &applicable_ops);
};

#endif
