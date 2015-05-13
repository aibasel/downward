#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "abstract_search.h"
#include "split_selector.h"
#include "split_tree.h"

#include "../countdown_timer.h"
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
struct Flaw;

const int STATES_LOG_STEP = 1000;

class Abstraction {
    const TaskProxy task_proxy;
    const bool do_separate_unreachable_facts;

    AbstractSearch abstract_search;

    // Strategy for picking the next flaw in case of multiple possibilities.
    SplitSelector split_selector;

    const int max_states;

    // Limit the time for building the abstraction.
    CountdownTimer timer;

    const State concrete_initial_state;

    // Set of all valid states, i.e. states that have not been split.
    AbstractStates states;

    // Abstract init and goal states.
    AbstractState *init;
    AbstractStates goals;

    // Statistics.
    int deviations;
    int unmet_preconditions;
    int unmet_goals;

    // Refinement hierarchy containing two child states for each split state.
    SplitTree split_tree;

    void create_trivial_abstraction();
    void separate_unreachable_facts();
    bool may_keep_refining() const;

    // Build abstraction.
    void build();

    bool is_goal(AbstractState *state) const;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    AbstractState get_cartesian_set(const ConditionsProxy &conditions) const;

    // Try to convert the abstract solution into a concrete trace. If a flaw
    // is encountered, refine the abstraction in a way that prevents the flaw
    // from appearing in the next round again.
    std::shared_ptr<Flaw> find_flaw(const Solution &solution);

    // Make Dijkstra search to calculate all goal distances and update h-values.
    void update_h_values();

    void print_statistics();

public:
    explicit Abstraction(const Options &opts);
    ~Abstraction();

    Abstraction(const Abstraction &) = delete;
    Abstraction &operator=(const Abstraction &) = delete;

    SplitTree get_split_tree() {return split_tree; }

    int get_num_states() const {return states.size(); }

    // Methods for additive abstractions.
    // For each operator op from a1 to a2, set cost'(op) = max(h(a1)-h(a2), 0).
    // This makes the next abstraction additive to all previous ones.
    std::vector<int> get_needed_costs();

    int get_init_h() const;
};
}

#endif
