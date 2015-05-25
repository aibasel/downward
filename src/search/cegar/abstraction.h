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
    const int max_states;

    AbstractSearch abstract_search;
    SplitSelector split_selector;

    // Limit the time for building the abstraction.
    CountdownTimer timer;

    // Set of all abstract states, i.e. states that have not been split.
    AbstractStates states;

    // Abstract initial and goal states.
    AbstractState *init;
    AbstractStates goals;

    // Count the number of times each flaw type is encountered.
    int deviations;
    int unmet_preconditions;
    int unmet_goals;

    // Refinement hierarchy.
    SplitTree split_tree;

    void create_trivial_abstraction();

    // Map all states with unreachable facts to arbitrary goal states.
    void separate_unreachable_facts();

    bool may_keep_refining() const;

    // Build abstraction.
    void build();

    bool is_goal(AbstractState *state) const;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    AbstractState get_cartesian_set(const ConditionsProxy &conditions) const;

    /* Try to convert the abstract solution into a concrete trace. Return the
       first encountered flaw or nullptr if there is no flaw. */
    std::shared_ptr<Flaw> find_flaw(const Solution &solution);

    // Perform Dijkstra's algorithm from the goal states to update the h-values.
    void update_h_values();

    void print_statistics();

public:
    explicit Abstraction(const Options &opts);
    ~Abstraction();

    Abstraction(const Abstraction &) = delete;
    Abstraction &operator=(const Abstraction &) = delete;

    SplitTree && get_split_tree() {return std::move(split_tree); }

    int get_num_states() const {return states.size(); }

    /* For each operator calculate the mimimum cost that is needed to preserve
       all abstract goal distances. */
    std::vector<int> get_needed_costs();

    int get_h_value_of_initial_state() const;
};
}

#endif
