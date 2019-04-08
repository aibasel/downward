#ifndef PER_TASK_INFORMATION_H
#define PER_TASK_INFORMATION_H

#include "task_proxy.h"

#include "algorithms/subscriber.h"
#include "utils/hash.h"
#include "utils/memory.h"

#include <functional>

/*
  A PerTaskInformation<T> acts like a HashMap<TaskID, T>
  with two main differences:
  (1) If an entry is accessed that does not exist yet, it is created using a
      factory function that is passed to the PerTaskInformation in its
      constructor.
  (2) If a task is destroyed, its associated data in all PerTaskInformation
      objects is automatically destroyed as well.

*/
template<class Entry>
class PerTaskInformation : public subscriber::Subscriber<AbstractTask> {
    /*
      EntryConstructor is the type of a function that can create objects for
      a given task if the PerTaskInformation is accessed for a task that has no
      associated entry yet. It receives a TaskProxy instead of the AbstractTask
      because AbstractTask is an internal implementation detail as far as other
      classes are concerned. It should return a unique_ptr to the newly created
      object.
    */
    using EntryConstructor = std::function<std::unique_ptr<Entry>(const TaskProxy &)>;
    EntryConstructor entry_constructor;
    utils::HashMap<TaskID, std::unique_ptr<Entry>> entries;
public:
    /*
      If no entry_constructor is passed to the PerTaskInformation explicitly,
      we assume the class Entry has a constructor that takes a single TaskProxy
      parameter.
    */
    PerTaskInformation()
        : entry_constructor(
              [](const TaskProxy &task_proxy) {
                  return utils::make_unique_ptr<Entry>(task_proxy);
              }) {
    }

    explicit PerTaskInformation(EntryConstructor entry_constructor)
        : entry_constructor(entry_constructor) {
    }

    Entry &operator[](const TaskProxy &task_proxy) {
        TaskID id = task_proxy.get_id();
        const auto &it = entries.find(id);
        if (it == entries.end()) {
            entries[id] = entry_constructor(task_proxy);
            task_proxy.subscribe_to_task_destruction(this);
        }
        return *entries[id];
    }

    virtual void notify_service_destroyed(const AbstractTask *task) override {
        TaskID id = TaskProxy(*task).get_id();
        entries.erase(id);
    }
};

#endif
