#include "../../../src/search/global_task_interface.h"
#include "../../../src/search/globals.h"
#include "../../../src/search/task.h"

#include <ctime>
#include <functional>
#include <iostream>
#include <string>

using namespace std;


void benchmark(const string &desc, int num_calls,
               const function<void ()> &func) {
    cout << "Running " << desc << " " << num_calls << " times:" << flush;
    clock_t start = clock();
    for (int i = 0; i < num_calls; ++i)
        func();
    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    cout << " " << duration << " seconds" << endl;
}


int main(int, char **) {
    const int NUM_ITERATIONS = 10000;
    read_everything(cin);

    Task *task = new Task(new GlobalTaskInterface);

    benchmark("nothing", NUM_ITERATIONS, [] (){}
    );
    cout << endl;
    benchmark("standard loop over operators", NUM_ITERATIONS, [&]() {
        Operators ops = task->get_operators();
        size_t num_ops = ops.size();
        int total_cost = 0;
        for (size_t i = 0; i < num_ops; ++i) {
            total_cost += ops[i].get_cost();
        }
    });
    benchmark("range-based loop over operators", NUM_ITERATIONS, [&]() {
        int total_cost = 0;
        for (Operator op : task->get_operators()) {
            total_cost += op.get_cost();
        }
    });
    return 0;
}
