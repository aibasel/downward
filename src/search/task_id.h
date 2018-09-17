#ifndef TASK_ID_H
#define TASK_ID_H

#include <cstdint>
#include <iostream>

class AbstractTask;

/*
  This class gives public access to an identify for a task proxy (for
  comparison, maps and unordered_maps) without publicly exposing the internal
  AbstractTask *.
*/
class TaskID {
    uintptr_t value;
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

    size_t hash() const {
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
