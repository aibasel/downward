#ifndef TASK_UTILS_ON_DEMAND_TASK_OBJECT
#define TASK_UTILS_ON_DEMAND_TASK_OBJECT

#include "../task_proxy.h"

#include "../utils/memory.h"

namespace task_utils {
template<typename T>
class OnDemandTaskObject {
    mutable std::unique_ptr<T> instance = nullptr;
public:
    const T &get(const TaskProxy &task_proxy) const {
        if (!instance) {
            instance = utils::make_unique_ptr<T>(task_proxy);
        }
        return *instance;
    }
};
}

#endif
