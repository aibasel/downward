#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "state_id.h"

#include "algorithms/int_packer.h"

#include <cstddef>
#include <iostream>
#include <vector>

class StateRegistry;

using PackedStateBin = int_packer::IntPacker::Bin;

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class GlobalState {
    friend class StateRegistry;
    template<typename Entry>
    friend class PerStateInformation;

    // Values for vars are maintained in a packed state and accessed on demand.
    const PackedStateBin *buffer;

    // registry isn't a reference because we want to support operator=
    const StateRegistry *registry;
    StateID id;

    // Only used by the state registry.
    GlobalState(
        const PackedStateBin *buffer, const StateRegistry &registry, StateID id);

    const PackedStateBin *get_packed_buffer() const {
        return buffer;
    }

    const StateRegistry &get_registry() const {
        return *registry;
    }
public:
    ~GlobalState() = default;

    StateID get_id() const {
        return id;
    }

    int operator[](int var) const;

    std::vector<int> get_values() const;

    void dump_pddl() const;
    void dump_fdr() const;
};


#endif
