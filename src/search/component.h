#ifndef FAST_DOWNWARD_COMPONENT_H
#define FAST_DOWNWARD_COMPONENT_H

#include "utils/logging.h"

#include <memory>
#include <unordered_map>

class Component {
public:
    virtual ~Component() = default;
};

class TaskIndependentComponent {
protected:
    const std::string name;
    const utils::Verbosity verbosity;
    mutable utils::LogProxy log;
    virtual std::string get_product_name() const = 0;
public:
    explicit TaskIndependentComponent(const std::string &name,
                                      utils::Verbosity verbosity) : name(name),
                                      verbosity(verbosity), log(utils::get_log_from_verbosity(verbosity)){}
    virtual ~TaskIndependentComponent() = default;
};

using ComponentMap = std::unordered_map<const TaskIndependentComponent *, std::shared_ptr<Component>>;
#endif
