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
    const std::string description;
    const utils::Verbosity verbosity;
    mutable utils::LogProxy log;
    virtual std::string get_product_name() const = 0;
public:
    std::string get_description() {return description;}
    explicit TaskIndependentComponent(const std::string &description, utils::Verbosity verbosity)
        : description(description), verbosity(verbosity), log(utils::get_log_for_verbosity(verbosity)) {}
    virtual ~TaskIndependentComponent() = default;
};

using ComponentMap = std::unordered_map<const TaskIndependentComponent *, std::shared_ptr<Component>>;
#endif
