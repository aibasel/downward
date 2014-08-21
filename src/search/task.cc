#include "task.h"

using namespace std;

Variable Fact::get_variable() const {
    return Variable(interface, var_id);
}
