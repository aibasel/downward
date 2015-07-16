#include "successor_generator.h"

#include "task_tools.h"

#include <cstdlib>
#include <iostream>
#include <vector>
using namespace std;

SuccessorGenerator::SuccessorGenerator(const shared_ptr<AbstractTask> task)
    : task(task),
      task_proxy(*task) {
}

/*
  TODO: this is a dummy implementation that will be replaced with code from the
  preprocessor in issue547. For now, we just loop through operators every time.
*/
void SuccessorGenerator::generate_applicable_ops(
    const State &state, std::vector<OperatorProxy> &applicable_ops) {
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (is_applicable(op, state)) {
            applicable_ops.push_back(op);
        }
    }
}
