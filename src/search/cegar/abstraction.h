#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "abstract_state.h"
#include "flaw_selector.h"
#include "split_tree.h"

#include "../countdown_timer.h"
#include "../priority_queue.h"
#include "../task_proxy.h"

#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class AdditiveHeuristic;
class Options;

namespace cegar {
class AbstractState;
class DomainAbstractedTask;

typedef std::unordered_set<AbstractState *> AbstractStates;

const int STATES_LOG_STEP = 1000;

class Abstraction {
private:
    const TaskProxy task_proxy;

    // Strategy for picking the next flaw in case of multiple possibilities.
    FlawSelector flaw_selector;

    int max_states;
    bool use_astar;
    bool use_general_costs;
    bool write_graphs;

    // Limit the time for building the abstraction.
    CountdownTimer timer;

    const State concrete_initial_state;

    // Set of all valid states, i.e. states that have not been split.
    AbstractStates states;

    // Abstract init and goal states.
    AbstractState *init;
    AbstractStates goals;

    // Queue for A* search.
    mutable AdaptiveQueue<AbstractState *> open_queue;
    mutable std::unordered_map<AbstractState *, Arc> solution_backward;
    mutable std::unordered_map<AbstractState *, Arc> solution_forward;

    // Statistics.
    mutable int deviations;
    mutable int unmet_preconditions;
    mutable int unmet_goals;

    // Refinement hierarchy containing two child states for each split state.
    SplitTree split_tree;

    int get_min_goal_distance() const;
    bool is_goal(AbstractState *state) const;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    // A* search.
    void reset_distances_and_solution() const;
    void astar_search(bool forward, bool use_h, std::vector<int> *needed_costs = 0) const;

    // Refine states between state and init until the solution is broken.
    void break_solution(AbstractState *state, const Splits &splits);

    // Try to convert the abstract solution into a concrete trace. If a flaw
    // is encountered, refine the abstraction in a way that prevents the flaw
    // from appearing in the next round again.
    bool check_and_break_solution(State conc_state, AbstractState *abs_state);

    // Make Dijkstra search to calculate all goal distances and update h-values.
    void update_h_values() const;

    void extract_solution(AbstractState *goal) const;
    void find_solution() const;

    // Testing.
    void write_dot_file(int num);

public:
    explicit Abstraction(const Options &opts);
    ~Abstraction();

    Abstraction(const Abstraction &) = delete;
    Abstraction &operator=(const Abstraction &) = delete;

    void separate_unreachable_facts();

    // Build abstraction.
    void build();

    SplitTree get_split_tree() {return split_tree; }

    int get_num_states() const {return states.size(); }
    bool may_keep_refining() const;

    // Methods for additive abstractions.
    // For each operator op from a1 to a2, set cost'(op) = max(h(a1)-h(a2), 0).
    // This makes the next abstraction additive to all previous ones.
    std::vector<int> get_needed_costs();

    // Statistics.
    void print_statistics();
    int get_init_h() const;
};
}

#endif
