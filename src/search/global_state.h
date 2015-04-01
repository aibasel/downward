#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "int_packer.h"
#include "state_id.h"

#include <cstddef>
#include <iostream>
#include <vector>

class GlobalOperator;
class StateRegistry;

typedef IntPacker::Bin PackedStateBin;

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class GlobalState {
    friend class StateRegistry;
    template <class Entry>
    friend class PerStateInformation;
    // Values for vars are maintained in a packed state and accessed on demand.
    const PackedStateBin *buffer;
    // registry isn't a reference because we want to support operator=
    const StateRegistry *registry;
    StateID id;
    // Only used by the state registry.
    GlobalState(const PackedStateBin *buffer_, const StateRegistry &registry_,
                StateID id_);

    const PackedStateBin *get_packed_buffer() const {
        return buffer;
    }

    const StateRegistry &get_registry() const {
        return *registry;
    }

    // No implementation to prevent default construction
    GlobalState();
public:
    ~GlobalState();

    StateID get_id() const {
        return id;
    }

    int operator[](std::size_t index) const;

    void dump_pddl() const;
    void dump_fdr() const;
};

#endif
