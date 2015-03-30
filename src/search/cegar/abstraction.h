#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "split_tree.h"

#include "../global_state.h"
#include "../priority_queue.h"
#include "../rng.h"
#include "../timer.h"

#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class StateRegistry;

namespace cegar {
class AbstractState;
class LandmarkTask;

typedef std::unordered_set<AbstractState *> AbstractStates;

const int STATES_LOG_STEP = 1000;

// In case there are multiple unment conditions, how do we choose the next one?
enum PickStrategy {
    RANDOM,
    // Number of remaining values for each variable.
    // "Constrainment" is bigger if there are less remaining possible values.
    MIN_CONSTRAINED,
    MAX_CONSTRAINED,
    // Refinement: - (remaining_values / original_domain_size)
    MIN_REFINED,
    MAX_REFINED,
    // Compare the h^add(s_0) values of the facts.
    MIN_HADD,
    MAX_HADD
};

class Abstraction {
private:
    // Forbid copy constructor and copy assignment operator.
    Abstraction(const Abstraction &);
    Abstraction &operator=(const Abstraction &);

    const TaskProxy task_proxy;
    std::unordered_map<const GlobalOperator *, int> op_to_index;
    StateRegistry *registry;
    std::shared_ptr<AdditiveHeuristic> additive_heuristic;

    // Set of all valid states, i.e. states that have not been split.
    AbstractStates states;

    // Root of the refinement hierarchy.
    AbstractState *single;

    // Abstract init and goal states.
    AbstractState *init;
    AbstractStates goals;

    // Queue for A* search.
    mutable AdaptiveQueue<AbstractState *> *open;

    // How to pick the next split in case of multiple possibilities.
    PickStrategy pick;
    mutable RandomNumberGenerator rng;

    std::vector<int> fact_positions_in_lm_graph_ordering;

    // Statistics.
    mutable int num_states;
    mutable int deviations;
    mutable int unmet_preconditions;
    mutable int unmet_goals;

    // Settings.
    int max_states;
    // Maximum time for building the abstraction.
    int max_time;
    bool use_astar;
    bool use_general_costs;
    bool dump_graphs;

    // Save whether the states have been destroyed.
    bool memory_released;

    // The timer is started when we begin building the abstraction.
    Timer timer;

    // Refinement hierarchy containing two child states for each split state.
    SplitTree split_tree;

    int get_min_goal_distance() const;
    bool is_goal(AbstractState *state) const;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    // Pick a possible split in case of multiple possibilities.
    int pick_split_index(AbstractState &state, const Splits &conditions) const;

    // A* search.
    void reset_distances_and_solution() const;
    bool astar_search(bool forward, bool use_h, std::vector<int> *needed_costs = 0) const;

    // Refine states between state and init until the solution is broken.
    void break_solution(AbstractState *state, const Splits &splits);

    // Try to convert the abstract solution into a concrete trace. If a flaw
    // is encountered, refine the abstraction in a way that prevents the flaw
    // from appearing in the next round again.
    bool check_and_break_solution(GlobalState conc_state, AbstractState *abs_state);

    // Make Dijkstra search to calculate all goal distances and update h-values.
    void update_h_values() const;

    void extract_solution(AbstractState *goal) const;
    void find_solution() const;

public:
    Abstraction(TaskProxy task_proxy,
                std::shared_ptr<AdditiveHeuristic> additive_heuristic);
    ~Abstraction();

    void separate_unreachable_facts(const std::unordered_set<FactProxy> &reachable_facts);

    // Build abstraction.
    void build();

    int get_h(const int *buffer) const {return split_tree.get_node(buffer)->get_h(); }

    int get_num_states() const {return num_states; }
    bool may_keep_refining() const;

    // Destroy all states when termination criterion is met.
    void release_memory();
    bool has_released_memory() const {return memory_released; }

    // Methods for additive abstractions.
    int get_op_index(const GlobalOperator *op) const;
    // For each operator op from a1 to a2, set cost'(op) = max(h(a1)-h(a2), 0).
    // This makes the next abstraction additive to all previous ones.
    void get_needed_costs(std::vector<int> *needed_costs);

    // Statistics.
    void print_statistics();
    void print_histograms() const;
    int get_init_h() const;

    // Settings.
    void set_max_states(int states) {max_states = states; }
    void set_max_time(int time) {max_time = time; }
    void set_use_astar(bool astar) {use_astar = astar; }
    void set_use_general_costs(bool negative) {use_general_costs = negative; }
    void set_pick_strategy(PickStrategy strategy);
    void set_dump_graphs(bool write) {dump_graphs = write; }

    // Testing.
    void write_dot_file(int num);
};
}

#endif
