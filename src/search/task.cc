#include "task.h"

using namespace std;

Variable Fact::get_variable() const {
    return Variable(impl, var_id);
}

bool OperatorRef::is_applicable(const StateRef &state) const {
    return impl.operator_is_applicable_in_state(index, state.get_index());
}
