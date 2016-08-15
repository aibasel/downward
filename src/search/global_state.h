#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "int_packer.h"
#include "state_id.h"

#include <cstddef>
#include <iostream>
#include <vector>

class AbstractTask;
class GlobalOperator;
class StateRegistry;

using PackedStateBin = IntPacker::Bin;

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

    int num_variables;

    // Only used by the state registry.
    GlobalState(const PackedStateBin *buffer,
                const StateRegistry &registry,
                StateID id,
                int num_variables);

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
};

extern void dump_pddl(const GlobalState &global_state, const AbstractTask &task);
extern void dump_fdr(const GlobalState &global_state, const AbstractTask &task);

#endif
