#include "open_list_factory.h"

#include "open_list.h"

#include "../state_id.h"

using namespace std;


template<>
unique_ptr<OpenList<StateID> > OpenListFactory::create_open_list() {
    return create_state_open_list();
}
