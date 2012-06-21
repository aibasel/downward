#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "abstract_state.h"
#include "utils.h"
#include "../operator.h"
#include "../priority_queue.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <vector>
#include <deque>
#include <limits>
#include <set>
#include <string>
#include <utility>

namespace cegar_heuristic {
const int INFINITY = numeric_limits<int>::max();

enum PickStrategy {
    FIRST,
    RANDOM,
    GOAL
};

class Abstraction {
private:
    std::vector<AbstractState *> abs_states;

    AbstractState *init;
    std::deque<AbstractState *> solution_states;
    std::deque<Operator *> solution_ops;

    PickStrategy pick_strategy;
    void pick_condition(const std::vector<std::pair<int, int> > &conditions,
                        int *var, int *value) const;

    bool dijkstra_search(HeapQueue<AbstractState *> &queue, bool forward);
    void extract_solution(AbstractState &goal);

    // Refinement hierarchy.
    AbstractState *single;
    void collect_states();
    void collect_child_states(AbstractState *parent);

public:
    explicit Abstraction(PickStrategy strategy = FIRST);

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
    std::vector<AbstractState *> *get_abs_states() {return &abs_states; }
    void write_dot_file(int num);
};
}

#endif
