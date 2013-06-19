#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "split_tree.h"
#include "../priority_queue.h"
#include "../rng.h"
#include "../state.h"
#include "../timer.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class AdditiveHeuristic;

namespace cegar_heuristic {
class AbstractState;

typedef unordered_set<AbstractState *> AbstractStates;

const int STATES_LOG_STEP = 1000;

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
    MAX_PREDECESSORS,
    // Only for cegar_sum.
    BEST2,
    // Choose the variable whose split yields the min/max number of new operators.
    MIN_OPS,
    MAX_OPS,
    // Compare the h^add values of the facts.
    MIN_HADD,
    MAX_HADD
};

class Abstraction {
private:
    // Forbid copy constructor and copy assignment operator.
    Abstraction(const Abstraction &);
    Abstraction &operator=(const Abstraction &);

    // Set of all valid states, i.e. states that have not been split.
    AbstractStates states;

    // Root of the refinement hierarchy.
    AbstractState *single;

    // Abstract init and goal state. There will always be only one goal state.
    AbstractState *init;
    AbstractState *goal;

    // Queue for A* search.
    mutable AdaptiveQueue<AbstractState *> *open;

    // How to pick the next split in case of multiple possibilities.
    PickStrategy pick;
    mutable RandomNumberGenerator rng;

    // Members for additive abstractions.
    mutable vector<int> needed_operator_costs;

    AdditiveHeuristic *hadd;

    // Statistics.
    mutable int num_states;
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
    bool log_h;

    // A* modes.
    bool calculate_needed_operator_costs;

    // Save whether the states have been destroyed.
    bool memory_released;

    // The timer is started when we begin building the abstraction.
    Timer timer;

    // Refinement hierarchy containing two child states for each split state.
    SplitTree split_tree;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    // Pick a possible split in case of multiple possibilities.
    int pick_split_index(AbstractState &state, const Splits &conditions) const;

    // A* search.
    void reset_distances_and_solution() const;
    bool breadth_first_search(queue<AbstractState *> &open_queue, bool forward) const;
    FRIEND_TEST(CegarTest, dijkstra_search);
    bool dijkstra_search(bool forward) const;

    // Refine states between state and init until the solution is broken.
    void break_solution(AbstractState *state, const Splits &splits);

    // Try to convert the abstract solution into a concrete trace. If a flaw
    // is encountered, refine the abstraction in a way that prevents the flaw
    // from appearing in the next round again.
    bool check_and_break_solution(State conc_state, AbstractState *abs_state = 0);

    // Make Dijkstra search to calculate all goal distances and update h-values.
    void update_h_values() const;
    void log_h_values() const;

public:
    Abstraction();
    ~Abstraction();

    // Build abstraction offline.
    void build();

    // Refine until the h-value for state improves.
    AbstractState *improve_h(const State &state, AbstractState *abs_state);

    AbstractState *get_abstract_state(const State &state) const;
    int get_h(const State &state) const {return split_tree.get_node(state)->get_h(); }

    int get_num_states() const {return num_states; }
    int get_num_states_online() const;
    int get_max_states_online() const {return max_states_online; }
    bool is_online() const {return num_states_offline != -1; }
    bool may_keep_refining() const;

    // Destroy all states when termination criterion is met.
    void release_memory();
    bool has_released_memory() const {return memory_released; }

    // Methods for additive abstractions.
    int get_op_index(const Operator *op) const;
    // For each operator op from a1 to a2, set cost'(op) = max(h(a1)-h(a2), 0).
    // This makes the next abstraction additive to all previous ones.
    void adapt_operator_costs();

    // Statistics.
    void print_statistics();
    double get_avg_h() const;

    // Settings.
    void set_max_states_offline(int states) {max_states_offline = states; }
    void set_max_states_online(int states) {max_states_online = states; }
    void set_max_time(int time) {max_time = time; }
    void set_log_h(bool log) {log_h = log; }
    void set_pick_strategy(PickStrategy strategy) {pick = strategy; }
    void set_hadd(AdditiveHeuristic *h);

    // Testing.
    void write_dot_file(int num);
};
}

#endif
