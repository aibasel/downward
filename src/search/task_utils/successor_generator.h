#ifndef TASK_UTILS_SUCCESSOR_GENERATOR_H
#define TASK_UTILS_SUCCESSOR_GENERATOR_H

#include "../task_proxy.h"

#include <list>
#include <memory>
#include <vector>

class GlobalState;

namespace successor_generator {
class GeneratorBase;

class SuccessorGenerator {
    std::unique_ptr<GeneratorBase> root;

public:
    explicit SuccessorGenerator(const TaskProxy &task_proxy);
    /*
      Note that the destructor cannot be set to the default destructor here
      (~SuccessorGenerator() = default;) because GeneratorBase is a forward
      declaration and the incomplete type cannot be destroyed.
    */
    ~SuccessorGenerator();

    void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const;
};
}

#endif
