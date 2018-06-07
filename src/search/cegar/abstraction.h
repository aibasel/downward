#ifndef CEGAR_ABSTRACTION_H
#define CEGAR_ABSTRACTION_H

#include "types.h"

#include "../task_proxy.h"

#include "../utils/collections.h"

#include <memory>
#include <vector>

namespace cegar {
class AbstractState;
class RefinementHierarchy;
class TransitionSystem;

/*
  Store the set of AbstractStates, use AbstractSearch to find abstract
  solutions, find flaws, use SplitSelector to select splits in case of
  ambiguities, break spurious solutions and maintain the
  RefinementHierarchy.
*/
class Abstraction {
    const std::unique_ptr<TransitionSystem> transition_system;
    const State concrete_initial_state;
    const std::vector<FactPair> goal_facts;

    // All (as of yet unsplit) abstract states.
    AbstractStates states;
    // Abstract initial state.
    AbstractState *init;
    // Abstract goal states. Only landmark tasks can have multiple goal states.
    Goals goals;

    /* DAG with inner nodes for all split states and leaves for all
       current states. */
    std::unique_ptr<RefinementHierarchy> refinement_hierarchy;

    const bool debug;

    void initialize_trivial_abstraction(const std::vector<int> &domain_sizes);

public:
    Abstraction(const std::shared_ptr<AbstractTask> &task, bool debug);
    ~Abstraction();

    Abstraction(const Abstraction &) = delete;

    int get_num_states() const;
    const AbstractStates &get_states() const;
    AbstractState *get_initial_state() const;
    const Goals &get_goals() const;
    AbstractState *get_state(int state_id) const;
    const TransitionSystem &get_transition_system() const;
    int get_init_h() const;
    std::unique_ptr<RefinementHierarchy> extract_refinement_hierarchy();

    /* Needed for CEGAR::separate_facts_unreachable_before_goal(). */
    void mark_all_states_as_goals();

    // Split state into two child states.
    void refine(AbstractState *state, int var, const std::vector<int> &wanted);

    void print_statistics() const;
};
}

#endif
