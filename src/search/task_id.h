#ifndef TASK_ID_H
#define TASK_ID_H

#include <cstdint>
#include <functional>

class AbstractTask;

/*
  A TaskID uniquely identifies a task (for unordered_maps and comparison)
  without publicly exposing the internal AbstractTask pointer.
*/
class TaskID {
    const std::uintptr_t value;
public:
    explicit TaskID(const AbstractTask *task)
        : value(reinterpret_cast<uintptr_t>(task)) {
    }
    TaskID() = delete;

    bool operator==(const TaskID &other) const {
        return value == other.value;
    }

    bool operator!=(const TaskID &other) const {
        return !(*this == other);
    }

    std::size_t hash() const {
        return value;
    }
};


namespace std {
template<>
struct hash<TaskID> {
    size_t operator()(TaskID id) const {
        return id.hash();
    }
};
}

#endif
