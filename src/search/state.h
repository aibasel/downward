#ifndef STATE_H
#define STATE_H

#include "globals.h"
#include "packed_state_entry.h"
#include "state_id.h"

#include <iostream>
#include <vector>

class Operator;
class StateRegistry;

struct PackedVariable {
    int index;
    int shift;
    PackedStateEntry read_mask;
    PackedStateEntry clear_mask;
};

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class State {
    static int packed_size;
    static std::vector<PackedVariable> packed_variables;

    friend class StateRegistry;
    template <class Entry>
    friend class PerStateInformation;
    // Values for vars are maintained in a packed state and accessed on demand.
    const PackedStateEntry *buffer;
    // registry isn't a reference because we want to support operator=
    const StateRegistry *registry;
    StateID id;
    // Only used by the state registry.
    State(const PackedStateEntry *buffer_, const StateRegistry &registry_,
          StateID id_);

    const PackedStateEntry *get_packed_buffer() const {
        return buffer;
    }

    const StateRegistry &get_registry() const {
        return *registry;
    }

    // No implementation to prevent default construction
    State();
public:
    ~State();

    StateID get_id() const {
        return id;
    }

    int operator[](int index) const;

    void dump_pddl() const;
    void dump_fdr() const;

    static int get(const PackedStateEntry *buffer, int index);
    static void set(PackedStateEntry *buffer, int index, int value);
    static void calculate_packed_size();
};

#endif
