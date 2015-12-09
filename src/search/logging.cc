#include "logging.h"

#include "globals.h"
#include "timer.h"
#include "utilities.h"

using namespace std;

void log_time_and_memory() {
    cout << "[t=" << g_timer << ", "
         << get_peak_memory_in_kb() << " KB]" << endl;
}


void log(const string &msg) {
    cout << msg << " ";
    log_time_and_memory();
}
