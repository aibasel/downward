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
const bool TEST_WITH_DIJKSTRA = false;

enum PickStrategy {
    FIRST,
    RANDOM,
    GOAL,
    NO_GOAL,
    MIN_CONSTRAINED,
    MAX_CONSTRAINED,
    MIN_REFINED,
    MAX_REFINED,
    MIN_PREDECESSORS,
    MAX_PREDECESSORS,
    BREAK,
    KEEP,
    ALL
};

class Abstraction {
private:
    std::set<AbstractState *> states;

    AbstractState *init;
    AbstractState *goal;
    AbstractState *start_solution_check_ptr;
    State last_checked_conc_state;

    PickStrategy pick_deviation;
    PickStrategy pick_precondition;
    PickStrategy pick_goal;
    void pick_condition(AbstractState &state, const std::vector<std::pair<int, int> > &conditions,
                        const PickStrategy &pick, int *var, int *value) const;

    void reset_distances() const;
    FRIEND_TEST(CegarTest, astar_search);
    bool astar_search(HeapQueue<AbstractState *> &queue, bool forward, bool use_h) const;
    FRIEND_TEST(CegarTest, dijkstra_search);
    bool dijkstra_search(HeapQueue<AbstractState *> &queue, bool forward) const;
    void extract_solution(AbstractState &goal) const;
    void calculate_costs() const;

    // Refinement hierarchy.
    AbstractState *single;

    vector<int> cg_partial_ordering;

    // Statistics.
    mutable int expansions;
    mutable int expansions_dijkstra;
    mutable int deviations;
    mutable int unmet_preconditions;
    mutable int unmet_goals;

public:
    explicit Abstraction(PickStrategy pick_deviation = FIRST,
                         PickStrategy pick_precondition = KEEP,
                         PickStrategy pick_goal = BREAK);

    void refine(AbstractState *state, int var, int value);
    void refine(vector<pair<int, int> > &conditions, AbstractState *state);

    AbstractState *get_abstract_state(const State &state) const;

    FRIEND_TEST(CegarTest, find_solution_first_state);
    FRIEND_TEST(CegarTest, find_solution_second_state);
    FRIEND_TEST(CegarTest, find_solution_loop);
    FRIEND_TEST(CegarTest, initialize);
    bool find_solution();

    std::string get_solution_string() const;
    bool can_reuse_last_solution() const {return start_solution_check_ptr; }
    bool check_solution();

    int get_num_expansions() const {return expansions; }
    int get_num_expansions_dijkstra() const {return expansions_dijkstra; }

    void update_h_values() const;

    int get_num_states() const {return states.size(); }

    void release_memory();
    void print_statistics();

    // Testing.
    void write_dot_file(int num);
};
}

#endif
