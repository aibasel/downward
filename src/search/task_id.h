#ifndef TASK_ID_H
#define TASK_ID_H

#include "utils/hash.h"

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

    std::uint64_t hash() const {
        return value;
    }
};


namespace utils {
inline void feed(HashState &hash_state, TaskID id) {
    feed(hash_state, id.hash());
}
}

#endif
