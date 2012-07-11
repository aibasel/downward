#include "state_manager.h"

using namespace std;
using namespace __gnu_cxx;

class StateManager::HashTable
    : public __gnu_cxx::hash_map<StateProxy, int> {
    // This is more like a typedef really, but we need a proper class
    // so that we can hide the information in the header file by using
    // a forward declaration. This is also the reason why the hash
    // table is allocated dynamically in the constructor.
};

StateManager::StateManager(): next_id(0) {
    states = new HashTable;
}


StateManager::~StateManager() {
    delete states;
}

StateManager &StateManager::get_instance() {
    // Instantiated on first use. Guaranteed to be destroyed.
    static StateManager instance;
    return instance;
}

int StateManager::get_id(const State &state) {
    StateProxy proxy(&state);
    HashTable::iterator it = states->find(proxy);
    if (it != states->end()) {
        return it->second;
    } else {
        int id = next_id++;
        states->insert(make_pair(proxy, id));
        return id;
    }
}
