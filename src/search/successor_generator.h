#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include "task_proxy.h"

#include <list>
#include <memory>
#include <vector>

class GeneratorBase;
class GlobalState;

class SuccessorGenerator {
    std::unique_ptr<GeneratorBase> root;

    SuccessorGenerator(const SuccessorGenerator &) = delete;
public:
    SuccessorGenerator(const TaskProxy &task_proxy);
    ~SuccessorGenerator();

    void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const;
};

#endif
