#ifndef PER_TASK_INFORMATION_H
#define PER_TASK_INFORMATION_H

#include "task_proxy.h"

#include "algorithms/subscriber.h"
#include "utils/memory.h"

#include <functional>
#include <unordered_map>

template<class Entry>
class PerTaskInformation : public subscriber::Subscriber<AbstractTask> {
    using EntryConstructor = std::function<std::unique_ptr<Entry>(const TaskProxy &)>;
    EntryConstructor entry_constructor;
    std::unordered_map<const AbstractTask *, std::unique_ptr<Entry>> entries;
public:
    PerTaskInformation()
        : entry_constructor(
              [](const TaskProxy &task_proxy) {
                  return utils::make_unique_ptr<Entry>(task_proxy);
              }) {
    }

    explicit PerTaskInformation(EntryConstructor default_constructor)
        : entry_constructor(default_constructor) {
    }

    Entry &operator[](const AbstractTask *task) {
        const auto &it = entries.find(task);
        if (it == entries.end()) {
            entries[task] = entry_constructor(TaskProxy(*task));
            task->subscribe(this);
        }
        return *entries[task];
    }

    virtual void notify_service_destroyed(const AbstractTask *task) override {
        entries.erase(task);
    }
};

#endif
