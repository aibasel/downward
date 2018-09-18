#include "globals.h"

#include "tasks/root_task.h"
#include "utils/timer.h"

#include <iostream>

using namespace std;

void read_everything(istream &in) {
    cout << "reading input... [t=" << utils::g_timer << "]" << endl;
    tasks::read_root_task(in);
    cout << "done reading input! [t=" << utils::g_timer << "]" << endl;
    cout << "done initializing global data [t=" << utils::g_timer << "]" << endl;
}

