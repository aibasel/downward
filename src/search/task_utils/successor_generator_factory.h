#ifndef TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H

#include <memory>
#include <vector>

struct FactPair;
class OperatorID;
class TaskProxy;

namespace successor_generator {
class GeneratorBase;

using GeneratorPtr = std::unique_ptr<GeneratorBase>;

// TODO: Consider declaring these two within SuccessorGeneratorFactory.
struct OperatorRange;
class OperatorInfo;


class SuccessorGeneratorFactory {
    // TODO: Consider using this representation instead:
    // using ValuesAndGenerators = std::vector<std::pair<int, GeneratorPtr>>;
    using ValuesAndGenerators = std::vector<GeneratorPtr>;

    const TaskProxy &task_proxy;
    std::vector<OperatorInfo> operator_infos;

    GeneratorPtr construct_chain(std::vector<GeneratorPtr> &nodes) const;
    GeneratorPtr construct_empty() const;
    GeneratorPtr construct_immediate(OperatorRange range) const;
    GeneratorPtr construct_switch(
        int switch_var_id, ValuesAndGenerators generator_for_value) const;
    GeneratorPtr construct_recursive(OperatorRange range);
public:
    explicit SuccessorGeneratorFactory(const TaskProxy &task_proxy);
    /* We cannot leave the destructor implicit because we only have a
       forward declaration of OperatorInfo. */
    ~SuccessorGeneratorFactory();
    GeneratorPtr create();
};
}

#endif
