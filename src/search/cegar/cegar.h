#ifndef CEGAR_CEGAR_H
#define CEGAR_CEGAR_H

#include "abstract_search.h"
#include "split_selector.h"

#include "../task_proxy.h"

#include "../utils/countdown_timer.h"

#include <memory>

namespace utils {
class RandomNumberGenerator;
}

namespace cegar {
class Abstraction;
struct Flaw;

/*
  Iteratively refine a Cartesian abstraction with counterexample-guided
  abstraction refinement (CEGAR).

  Store the abstraction, use AbstractSearch to find abstract solutions, find
  flaws, use SplitSelector to select splits in case of ambiguities and break
  spurious solutions.
*/
class CEGAR {
    const TaskProxy task_proxy;
    const std::vector<int> domain_sizes;
    const int max_states;
    const int max_non_looping_transitions;
    const SplitSelector split_selector;

    std::unique_ptr<Abstraction> abstraction;
    AbstractSearch abstract_search;

    // Limit the time for building the abstraction.
    utils::CountdownTimer timer;

    const bool debug;

    bool may_keep_refining() const;

    /*
      Map all states that can only be reached after reaching the goal
      fact to arbitrary goal states.

      We need this method only for landmark subtasks, but calling it
      for other subtasks with a single goal fact doesn't hurt and
      simplifies the implementation.
    */
    void separate_facts_unreachable_before_goal();

    /* Try to convert the abstract solution into a concrete trace. Return the
       first encountered flaw or nullptr if there is no flaw. */
    std::unique_ptr<Flaw> find_flaw(const Solution &solution);

    // Build abstraction.
    void refinement_loop(utils::RandomNumberGenerator &rng);

    void print_statistics();

public:
    CEGAR(
        const std::shared_ptr<AbstractTask> &task,
        int max_states,
        int max_non_looping_transitions,
        double max_time,
        PickSplit pick,
        utils::RandomNumberGenerator &rng,
        bool debug);
    ~CEGAR();

    CEGAR(const CEGAR &) = delete;

    std::unique_ptr<Abstraction> extract_abstraction();
};
}

#endif
