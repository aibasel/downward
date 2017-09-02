#ifndef TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H

#include <memory>
#include <vector>

class TaskProxy;

namespace successor_generator {
class GeneratorBase;

using GeneratorPtr = std::unique_ptr<GeneratorBase>;

struct OperatorRange;
class OperatorInfo;


class SuccessorGeneratorFactory {
    // TODO: Consider using this representation instead:
    // using ValuesAndGenerators = std::vector<std::pair<int, GeneratorPtr>>;
    using ValuesAndGenerators = std::vector<GeneratorPtr>;

    const TaskProxy &task_proxy;
    std::vector<OperatorInfo> operator_infos;

    GeneratorPtr construct_fork(std::vector<GeneratorPtr> nodes) const;
    GeneratorPtr construct_leaf(OperatorRange range) const;
    GeneratorPtr construct_switch(
        int switch_var_id, ValuesAndGenerators generator_for_value) const;
    GeneratorPtr construct_recursive(int depth, OperatorRange range);
public:
    explicit SuccessorGeneratorFactory(const TaskProxy &task_proxy);
    // Destructor cannot be implicit because OperatorInfo is forward-declared.
    ~SuccessorGeneratorFactory();
    GeneratorPtr create();
};
}

#endif
