#ifndef TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H

#include <list>
#include <memory>
#include <vector>

struct FactPair;
class OperatorID;
class TaskProxy;

namespace successor_generator {

class GeneratorBase;

using GeneratorPtr = std::unique_ptr<GeneratorBase>;

class SuccessorGeneratorFactory {
    using Condition = std::vector<FactPair>;
    using OperatorList = std::list<OperatorID>;
    // TODO: Later switch to the following pair-based representation?
    // using ValuesAndGenerators = std::vector<std::pair<int, GeneratorPtr>>;
    using ValuesAndGenerators = std::vector<GeneratorPtr>;

    const TaskProxy &task_proxy;
    std::vector<Condition> conditions;
    std::vector<Condition::const_iterator> next_condition_by_op;

    GeneratorPtr construct_chain(std::vector<GeneratorPtr> &nodes) const;
    GeneratorPtr construct_immediate(OperatorList operators) const;
    GeneratorPtr construct_switch(
        int switch_var_id, ValuesAndGenerators generator_for_value) const;
    GeneratorPtr construct_recursive(OperatorList operator_queue);
public:
    explicit SuccessorGeneratorFactory(const TaskProxy &task_proxy);
    GeneratorPtr create();
};

}

#endif
