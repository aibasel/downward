#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "../priority_queue.h"
#include "../state.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <deque>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cegar_heuristic {
const int INFINITY = std::numeric_limits<int>::max();

class AbstractState;

enum PickStrategy {
    FIRST,
    RANDOM,
    GOAL,
    NO_GOAL,
    // Number of remaining values for each variable.
    MIN_CONSTRAINED,
    MAX_CONSTRAINED,
    // Refinement: |remaining_values| / |domain|
    MIN_REFINED,
    MAX_REFINED,
    // Number of predecessors in causal graph.
    MIN_PREDECESSORS,
    MAX_PREDECESSORS
};

class Abstraction {
private:
    // Forbid copy constructor and copy assignment operator.
    Abstraction(const Abstraction &);
    Abstraction &operator=(const Abstraction &);

    std::set<AbstractState *> states;
    std::deque<State> seen_conc_states;

    AbstractState *init;
    AbstractState *goal;

    void refine(AbstractState *state, int var, int value);

    PickStrategy pick_deviation;
    PickStrategy pick_precondition;
    PickStrategy pick_goal;
    void pick_condition(AbstractState &state, const std::vector<std::pair<int, int> > &conditions,
                        const PickStrategy &pick, int *var, int *value) const;

    // A* search.
    mutable AdaptiveQueue<AbstractState *> queue;
    void reset_distances() const;
    FRIEND_TEST(CegarTest, astar_search);
    FRIEND_TEST(CegarTest, dijkstra_search);
    bool astar_search(bool forward, bool use_h) const;
    void extract_solution(AbstractState *goal) const;
    void calculate_costs() const;

    // Refinement hierarchy.
    AbstractState *single;

    std::vector<int> cg_partial_ordering;

    // Statistics.
    mutable int expansions;
    mutable int deviations;
    mutable int unmet_preconditions;
    mutable int unmet_goals;
    mutable int num_states_offline;
    mutable double last_avg_h;
    mutable int last_init_h;
    mutable int searches_from_init;
    mutable int searches_from_random_state;

    bool use_astar;
    bool use_new_arc_check;
    bool log_h;
    int num_seen_conc_states;

    bool memory_released;

public:
    explicit Abstraction(PickStrategy deviation_strategy = RANDOM,
                         PickStrategy precondition_strategy = RANDOM,
                         PickStrategy goal_strategy = RANDOM);

    void break_solution(AbstractState *state, int var, int value);
    void improve_h(const State &state, AbstractState *abs_state);

    AbstractState *get_abstract_state(const State &state) const;

    FRIEND_TEST(CegarTest, find_solution_first_state);
    FRIEND_TEST(CegarTest, find_solution_second_state);
    FRIEND_TEST(CegarTest, find_solution_loop);
    FRIEND_TEST(CegarTest, initialize);
    bool find_solution(AbstractState *start = 0);

    std::string get_solution_string() const;
    bool check_and_break_solution(State conc_state, AbstractState *abs_state = 0);
    bool find_and_break_complete_solution();
    bool find_and_break_solution();

    int get_num_expansions() const {return expansions; }

    void update_h_values() const;
    void log_h_values() const;

    int get_num_states() const {return states.size(); }
    void remember_num_states_offline() const {num_states_offline = states.size(); }
    int get_num_states_online() const;
    bool may_keep_refining_online() const;
    void remember_conc_state(const State &conc_state);
    const State &get_random_conc_state() const;

    bool has_released_memory() const {return memory_released; }
    void release_memory();
    void print_statistics();
    double get_avg_h() const;

    void set_use_astar(bool astar) {use_astar = astar; }
    void set_use_new_arc_check(bool new_check) {use_new_arc_check = new_check; }
    void set_log_h(bool log) {log_h = log; }
    void set_num_seen_conc_states(int num) {num_seen_conc_states = num; }

    // Testing.
    void write_dot_file(int num);
};
}

#endif
