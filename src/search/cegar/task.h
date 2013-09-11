#ifndef CEGAR_TASK_H
#define CEGAR_TASK_H

#include "utils.h"
#include "../operator.h"
#include "../state.h"

#include <vector>

namespace cegar_heuristic {
class Task {
private:

public:
    Task();

    vector<Fact> goal;
    vector<int> variable_domains;
    vector<Operator> operators;

    void install() const;

    static Task save_original_task();
};
}

#endif
