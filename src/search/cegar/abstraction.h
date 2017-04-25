#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "abstract_search.h"
#include "refinement_hierarchy.h"
#include "split_selector.h"
#include "transition_updater.h"

#include "../task_proxy.h"

#include "../utils/countdown_timer.h"

#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace utils {
class RandomNumberGenerator;
}

namespace cegar {
class AbstractState;
struct Flaw;

/*
  Store the set of AbstractStates, use AbstractSearch to find abstract
  solutions, find flaws, use SplitSelector to select splits in case of
  ambiguities, break spurious solutions and maintain the
  RefinementHierarchy.
*/
class Abstraction {
    const TaskProxy task_proxy;
    const int max_states;
    const int max_non_looping_transitions;
    const bool use_general_costs;

    AbstractSearch abstract_search;
    SplitSelector split_selector;
    TransitionUpdater transition_updater;

    // Limit the time for building the abstraction.
    utils::CountdownTimer timer;

    /*
      Set of all (as of yet unsplit) abstract states.

      TODO: Store states as unique_ptrs. C++11 doesn't really support
      unordered_sets of unique_ptrs, so we should probably use an
      unordered_map<AbstractState *, unique_ptr<AbstractState>> to
      allow for removing elements (see
      https://stackoverflow.com/questions/18939882).
    */
    AbstractStates states;

    // Abstract initial state.
    AbstractState *init;
    /* Abstract goal states. Landmark tasks may have multiple abstract
       goal states. */
    AbstractStates goals;

    // Count the number of times each flaw type is encountered.
    int deviations;
    int unmet_preconditions;
    int unmet_goals;

    /* DAG with inner nodes for all split states and leaves for all
       current states. */
    RefinementHierarchy refinement_hierarchy;

    const bool debug;

    void create_trivial_abstraction();

    /*
      Map all states that can only be reached after reaching the goal
      fact to arbitrary goal states.

      We need this method only for landmark subtasks, but calling it
      for other subtasks with a single goal fact doesn't hurt and
      simplifies the implementation.
    */
    void separate_facts_unreachable_before_goal();

    bool may_keep_refining() const;

    // Build abstraction.
    void build(utils::RandomNumberGenerator &rng);

    bool is_goal(AbstractState *state) const;

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    AbstractState get_cartesian_set(const ConditionsProxy &conditions) const;

    /* Try to convert the abstract solution into a concrete trace. Return the
       first encountered flaw or nullptr if there is no flaw. */
    std::unique_ptr<Flaw> find_flaw(const Solution &solution);

    // Perform Dijkstra's algorithm from the goal states to update the h-values.
    void update_h_and_g_values();

    void print_statistics();

public:
    Abstraction(
        const std::shared_ptr<AbstractTask> task,
        int max_states,
        int max_non_looping_transitions,
        double max_time,
        bool use_general_costs,
        PickSplit pick,
        utils::RandomNumberGenerator &rng,
        bool debug = false);
    ~Abstraction();

    Abstraction(const Abstraction &) = delete;

    RefinementHierarchy extract_refinement_hierarchy() {
        assert(refinement_hierarchy.get_root());
        return std::move(refinement_hierarchy);
    }

    int get_num_states() const {
        return states.size();
    }

    int get_num_non_looping_transitions() const {
        return transition_updater.get_num_non_loops();
    }

    /*
      For each operator calculate the mimimum cost that is needed to
      preserve the abstract goal distances of all reachable states.
    */
    std::vector<int> get_saturated_costs();

    int get_h_value_of_initial_state() const;
};
}

#endif
