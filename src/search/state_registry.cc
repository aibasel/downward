#include "state_registry.h"

using namespace std;
using namespace __gnu_cxx;

class StateRegistry::HashTable
    : public __gnu_cxx::hash_map<StateProxy, int> {
    // This is more like a typedef really, but we need a proper class
    // so that we can hide the information in the header file by using
    // a forward declaration. This is also the reason why the hash
    // table is allocated dynamically in the constructor.
};

StateRegistry::StateRegistry(): next_id(0) {
    states = new HashTable;
}


StateRegistry::~StateRegistry() {
    delete states;
}

int StateRegistry::get_id(const State &state) {
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
