#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <ext/hash_map>
#include "state.h"
#include "state_proxy.h"

class State;

class StateManager
{
private:
    class HashTable;
    HashTable *states;
    int next_id;
    StateManager();
    StateManager(StateManager const&);   // Don't implement to avoid copies of singleton
    void operator=(StateManager const&); // Don't implement to avoid copies of singleton
public:
    // Singleton for now. Could later be changed to use one instance for each problem
    // in this case the method State::get_id would also have to be changed.
    static StateManager &get_instance();
    ~StateManager();
    int get_id(const State &state);
};


#endif // STATE_MANAGER_H
