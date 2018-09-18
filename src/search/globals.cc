#include "globals.h"

#include "task_utils/task_properties.h"
#include "tasks/root_task.h"
#include "utils/logging.h"

#include <iostream>

using namespace std;

void read_everything(istream &in) {
    cout << "reading input... [t=" << utils::g_timer << "]" << endl;
    tasks::read_root_task(in);
    cout << "done reading input! [t=" << utils::g_timer << "]" << endl;
    cout << "done initializing global data [t=" << utils::g_timer << "]" << endl;
}

bool is_unit_cost() {
    assert(tasks::g_root_task);
    static bool is_unit_cost = task_properties::is_unit_cost(TaskProxy(*tasks::g_root_task));
    return is_unit_cost;
}

utils::Log g_log;
