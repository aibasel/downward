#include "state_handle.h"

#include "globals.h"
#include "state_var_t.h"
#include "utilities.h"

#include <algorithm>
using namespace std;

struct StateHandle::StateRepresentation {
    StateRepresentation();
    explicit StateRepresentation(const state_var_t *_data);
    // Fields are mutable so they can be changed after the handle is inserted
    // into a hash_set, i.e. the state registry
    mutable int id;
    // This will later be replaced by packed state data
    mutable state_var_t *data;
    void make_permanent(int _id) const;
};

StateHandle::StateRepresentation::StateRepresentation() :
    id(INVALID_HANLDE), data(0) {
}

StateHandle::StateRepresentation::StateRepresentation(const state_var_t *_data) :
    id(INVALID_HANLDE) {
    data = const_cast<state_var_t *>(_data);
}

void StateHandle::StateRepresentation::make_permanent(int _id) const {
    // Code duplication with State::copy_buffer_from will disappear when packed
    // states are introduced. State will then copy unpacked states while this
    // method will copy the packed representation.
    state_var_t *copy = new state_var_t[g_variable_domain.size()];
    // Update values affected by operator.
    // TODO: Profile if memcpy could speed this up significantly,
    //       e.g. if we do blind A* search.
    for (int i = 0; i < g_variable_domain.size(); i++)
        copy[i] = data[i];
    data = copy;
    id = _id;
}

StateHandle::StateHandle()
    : representation(new StateRepresentation()) {
    // do nothing
}

StateHandle::StateHandle(const state_var_t *data)
    : representation(new StateRepresentation(data)) {
    // do nothing
}

StateHandle::StateHandle(const StateHandle& other)
    : representation(new StateRepresentation(*(other.representation))) {
    // do nothing
}

const StateHandle& StateHandle::operator=(StateHandle other) {
    std::swap(this->representation, other.representation);
    return* this;
}

StateHandle::~StateHandle() {
    delete representation;
}

void StateHandle::make_permanent(int id) const {
    representation->make_permanent(id);
}

int StateHandle::get_id() const {
    return representation->id;
}

bool StateHandle::is_valid() const {
    return representation->id != INVALID_HANDLE;
}

const state_var_t *StateHandle::get_buffer() const {
    // This will later be replaced by a method unpacking the packed state
    // contained in representation
    return representation->data;
}

bool StateHandle::operator==(const StateHandle &other) const {
    if (is_valid() && other.is_valid()) {
        // If both handles are valid we can compare their ids.
        return get_id() == other.get_id();
    } else if (representation && other.representation) {
        // If at least one handle is invalid, but they both have a representation
        // we compare their representations semantically.
        // This case happens during state registration.
        int size = g_variable_domain.size();
        return ::equal(representation->data, representation->data + size,
                       other.representation->data);
    } else {
        // At least one of the handles contains no data. They are the same iff
        // both of them are empty.
        return !representation && !other.representation;
    }
}

size_t StateHandle::hash() const {
    if (!representation) {
        return 0;
    }
    return ::hash_number_sequence(representation->data, g_variable_domain.size());
}
