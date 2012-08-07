#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include <ext/hash_map>
#include "state.h"
#include "state_proxy.h"

class State;

class StateRegistry
{
private:
    class HashTable;
    HashTable *states;
    int next_id;
public:
    StateRegistry();
    ~StateRegistry();
    int get_id(const State &state);
};

#endif
