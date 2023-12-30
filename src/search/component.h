#ifndef FAST_DOWNWARD_COMPONENT_H
#define FAST_DOWNWARD_COMPONENT_H

#include <memory>
#include <unordered_map>

class Component {
public:
    virtual ~Component() = default;
};

class TaskIndependentComponent {
protected:
    virtual std::string get_product_name() const = 0;
public:
    explicit TaskIndependentComponent() {}
    virtual ~TaskIndependentComponent() = default;
};

using ComponentMap = std::unordered_map<const TaskIndependentComponent *, std::shared_ptr<Component>>;
#endif
