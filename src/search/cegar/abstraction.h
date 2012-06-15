#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include <vector>
#include <deque>
#include <set>
#include <string>
#include <utility>

#include "../operator.h"
#include "../priority_queue.h"
#include "abstract_state.h"
#include "utils.h"

#include "gtest/gtest_prod.h"

namespace cegar_heuristic {
extern int INFINITY;

class Abstraction {
private:
    std::vector<AbstractState *> abs_states;

    AbstractState *init;
    deque<AbstractState *> solution_states;
    deque<Operator *> solution_ops;

    void pick_condition(vector<pair<int, int> > &conditions, int *var, int *value) const;

    bool dijkstra_search(HeapQueue<AbstractState *> &queue, bool forward);
    void extract_solution(AbstractState &goal);

    // Refinement hierarchy.
    AbstractState *single;
    void collect_states();
    void collect_child_states(AbstractState *parent);

public:
    Abstraction();

    void refine(AbstractState *state, int var, int value);

    AbstractState *get_abstract_state(const State &state) const;

    FRIEND_TEST(CegarTest, find_solution_first_state);
    FRIEND_TEST(CegarTest, find_solution_second_state);
    FRIEND_TEST(CegarTest, find_solution_loop);
    FRIEND_TEST(CegarTest, initialize);
    bool find_solution();

    std::string get_solution_string() const;
    bool check_solution();

    void calculate_costs();

    // Only for testing.
    AbstractState *get_init() {return init; }
};
}

#endif
