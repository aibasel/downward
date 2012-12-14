#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "split_tree.h"
#include "../priority_queue.h"
#include "../rng.h"
#include "../state.h"
#include "../timer.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cegar_heuristic {
const int INFINITY = std::numeric_limits<int>::max();
const int STATES_LOG_STEP = 1000;
const int DEFAULT_H_UPDATE_STEP = 1000;

class AbstractState;

// In case there are multiple unment conditions, how do we choose the next one?
enum PickStrategy {
    FIRST,
    RANDOM,
    // Choose first variable that occurs/doesn't occur in the goal.
    GOAL,
    NO_GOAL,
    // Number of remaining values for each variable.
    // "Constrainment" is bigger if there are less remaining possible values.
    MIN_CONSTRAINED,
    MAX_CONSTRAINED,
    // Refinement: - (remaining_values / original_domain_size)
    MIN_REFINED,
    MAX_REFINED,
    // Number of predecessors in ordering of causal graph.
    MIN_PREDECESSORS,
    MAX_PREDECESSORS
};

class Abstraction {
private:
    // Forbid copy constructor and copy assignment operator.
    Abstraction(const Abstraction &);
    Abstraction &operator=(const Abstraction &);

    // Set of all valid states, i.e. states that have not been split.
    std::set<AbstractState *> states;

    // Root of the refinement hierarchy.
    AbstractState *single;

    // Abstract init and goal state. There will always be only one goal state.
    AbstractState *init;
    AbstractState *goal;

    // Queue for A* search.
    mutable AdaptiveQueue<AbstractState *> *queue;

    // How to pick the next split in case of multiple possibilities.
    PickStrategy pick;
    mutable RandomNumberGenerator rng;

    // Statistics.
    mutable int deviations;
    mutable int unmet_preconditions;
    mutable int unmet_goals;
    mutable int num_states_offline;
    mutable double last_avg_h;
    mutable int last_init_h;

    // Settings.
    int max_states_offline;
    int max_states_online;
    // Maximum time for building the abstraction.
    int max_time;
    bool use_astar;
    bool use_new_arc_check;
    bool log_h;

    // Save whether the states have been destroyed.
    bool memory_released;

    // The timer is started when we begin building the abstraction.
    Timer timer;

    // Refinement hierarchy containing two child states for each split state.
    SplitTree split_tree;

    // Ordering of the variables in the task's causal graph.
    std::vector<int> cg_partial_ordering;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    // Pick a possible split in case of multiple possibilities.
    int pick_split_index(AbstractState &state, const Splits &conditions) const;

    // A* search.
    void reset_distances() const;
    // Set the incoming and outgoing solution arcs for the states on the solution path.
    void extract_solution(AbstractState *goal) const;
    FRIEND_TEST(CegarTest, astar_search);
    FRIEND_TEST(CegarTest, dijkstra_search);
    bool astar_search(bool forward, bool use_h) const;
    FRIEND_TEST(CegarTest, find_solution_first_state);
    FRIEND_TEST(CegarTest, find_solution_second_state);
    FRIEND_TEST(CegarTest, find_solution_loop);
    FRIEND_TEST(CegarTest, initialize);
    bool find_solution(AbstractState *start = 0);

    // Refine states between state and init until the solution is broken.
    void break_solution(AbstractState *state, const Splits &splits);

    bool check_and_break_solution(State conc_state, AbstractState *abs_state = 0);
    bool find_and_break_solution();

    // Make Dijkstra search to calculate all goal distances and update h-values.
    void update_h_values() const;
    void log_h_values() const;

    void print_statistics();
    double get_avg_h() const;

public:
    Abstraction();
    ~Abstraction();

    // Build abstraction offline.
    void build(int h_updates);

    // Refine until the h-value for state improves.
    AbstractState *improve_h(const State &state, AbstractState *abs_state);

    AbstractState *get_abstract_state(const State &state) const;
    int get_h(const State &state) const {return split_tree.get_node(state)->get_h(); }

    int get_num_states() const {return states.size(); }
    int get_num_states_online() const;
    int get_max_states_online() const {return max_states_online; }
    bool is_online() const {return num_states_offline != -1; }
    bool may_keep_refining() const;

    // Destroy all states when termination criterion is met.
    void release_memory();
    bool has_released_memory() const {return memory_released; }

    // Settings.
    void set_max_states_offline(int states) {max_states_offline = states; }
    void set_max_states_online(int states) {max_states_online = states; }
    void set_max_time(int time) {max_time = time; }
    void set_use_astar(bool astar) {use_astar = astar; }
    void set_use_new_arc_check(bool new_check) {use_new_arc_check = new_check; }
    void set_log_h(bool log) {log_h = log; }
    void set_pick_strategy(PickStrategy strategy) {pick = strategy; }

    // Testing.
    std::string get_solution_string() const;
    void write_dot_file(int num);
};
}

#endif
