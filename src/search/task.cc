#include "task.h"


Fact::Fact(const TaskInterface &interface_, int var_id_, int value_)
    : interface(interface_), var_id(var_id_), value(value_) {
    assert(var_id >= 0 && var_id < interface.get_num_variables());
    assert(value >= 0 && value < get_variable().get_domain_size());
}


Variable Fact::get_variable() const {
    return Variable(interface, var_id);
}
