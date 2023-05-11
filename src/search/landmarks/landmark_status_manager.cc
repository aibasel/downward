#include "landmark_status_manager.h"

#include "landmark.h"

#include "../utils/logging.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks past, since we do an intersection when
  computing new landmark information.
*/
LandmarkStatusManager::LandmarkStatusManager(
    LandmarkGraph &graph,
    bool progress_goals,
    bool progress_greedy_necessary_orderings,
    bool progress_reasonable_orderings)
    : lm_graph(graph),
      progress_goals(progress_goals),
      progress_greedy_necessary_orderings(progress_greedy_necessary_orderings),
      progress_reasonable_orderings(progress_reasonable_orderings),
      past_landmarks(vector<bool>(graph.get_num_landmarks(), true)),
      future_landmarks(vector<bool>(graph.get_num_landmarks(), false)),
      lm_status(graph.get_num_landmarks(), FUTURE) {
}

BitsetView LandmarkStatusManager::get_past_landmarks(const State &state) {
    return past_landmarks[state];
}

BitsetView LandmarkStatusManager::get_future_landmarks(const State &state) {
    return future_landmarks[state];
}

void LandmarkStatusManager::process_initial_state(
    const State &initial_state, utils::LogProxy &log) {
    progress_initial_state(initial_state, log);
    update_lm_status(initial_state);
}

void LandmarkStatusManager::progress_initial_state(
    const State &initial_state, utils::LogProxy &/*log*/) {
    BitsetView past = get_past_landmarks(initial_state);
    BitsetView fut = get_future_landmarks(initial_state);

    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();
        const Landmark &lm = node->get_landmark();
        if (lm.is_true_in_state(initial_state)) {
            assert(past.test(id));
            for (auto &parent : node->parents) {
                /*
                  We assume here that no natural orderings A->B are generated
                  such that B holds in the initial state. Such orderings would
                  only be valid in unsolvable problems.
                */
                assert(parent.second <= EdgeType::REASONABLE);
                /*
                  Reasonable orderings A->B for which both A and B hold in the
                  initial state are immediately satisfied. We assume such
                  orderings are not generated in the first place.
                */
                assert(!parent.first->get_landmark().is_true_in_state(
                    initial_state));
                utils::unused_variable(parent);
                fut.set(id);
            }
        } else {
            past.reset(id);
            fut.set(id);
        }
    }
    /*
      TODO: The old code did some logging in this function. For example, it
       printed the number of landmarks that hold in the initial state or the
       goal. I personally don't think this is generally relevant information,
       but if we want to keep that output, it should be easy to add it again.

      TODO: If we decide to get rid of the logs, we can also remove the logger
       from the function header.
    */
}

void LandmarkStatusManager::progress(
    const State &parent_ancestor_state, OperatorID,
    const State &ancestor_state) {
    if (ancestor_state == parent_ancestor_state) {
        // This can happen, e.g., in Satellite-01.
        return;
    }

    const BitsetView parent_past = get_past_landmarks(parent_ancestor_state);
    BitsetView past = get_past_landmarks(ancestor_state);

    const BitsetView parent_fut = get_future_landmarks(parent_ancestor_state);
    BitsetView fut = get_future_landmarks(ancestor_state);

    int num_landmarks = lm_graph.get_num_landmarks();
    assert(past.size() == num_landmarks);
    assert(parent_past.size() == num_landmarks);
    assert(fut.size() == num_landmarks);
    assert(parent_fut.size() == num_landmarks);

    progress_basic(parent_past, parent_fut, parent_ancestor_state, past,
                   fut, ancestor_state);

    for (int id = 0; id < num_landmarks; ++id) {
        if (progress_goals) {
            progress_goal(id, ancestor_state, fut);
        }
        if (progress_greedy_necessary_orderings) {
            progress_greedy_necessary_ordering(id, ancestor_state, past, fut);
        }
        if (progress_reasonable_orderings) {
            progress_reasonable_ordering(id, past, fut);
        }
    }
}

void LandmarkStatusManager::progress_basic(
    const BitsetView &parent_past, const BitsetView &parent_fut,
    const State &parent_ancestor_state, BitsetView &past,
    BitsetView &fut, const State &ancestor_state) {

    utils::unused_variable(parent_past);
    utils::unused_variable(parent_ancestor_state);
    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();
        const Landmark &lm = node->get_landmark();
        if (parent_fut.test(id)) {
            /*
              A future landmark remains future if it is not achieved by the
              current transition. If it additionally wasn't past in the parent,
              it remains not past.

              We first test whether the landmark is currently considered past or
              not future. If neither holds, the code that follows the condition
              has no effect and we can therefore avoid computing whether the
              landmark holds in the current state.
              TODO: Computing whether a landmark holds in a state is expensive.
               It can happen that we compute this multiple times (considering
               also the ordering progressions that follow) for the same landmark
               and state. If we observe a slow-down in the experiments, we could
               look into this as an opportunity to speed things up.
            */
            if ((past.test(id) || !fut.test(id))
                && !lm.is_true_in_state(ancestor_state)) {
                fut.set(id);
                if (!parent_past.test(id)) {
                    past.reset(id);
                }
            }
        }
    }
}

void LandmarkStatusManager::progress_goal(
    int id, const State &ancestor_state, BitsetView &fut) {
    if (!fut.test(id)) {
        Landmark &lm = lm_graph.get_node(id)->get_landmark();
        if (lm.is_true_in_goal && !lm.is_true_in_state(ancestor_state)) {
            fut.set(id);
        }
    }
}

// TODO: Maybe store landmark orderings by type in landmark graph?
void LandmarkStatusManager::progress_greedy_necessary_ordering(
    int id, const State &ancestor_state, const BitsetView &past,
    BitsetView &fut) {
    if (past.test(id)) {
        return;
    }

    for (auto &parent : lm_graph.get_node(id)->parents) {
        if (parent.second != EdgeType::GREEDY_NECESSARY
            || fut.test(parent.first->get_id())) {
            continue;
        }
        if (!parent.first->get_landmark().is_true_in_state(ancestor_state)) {
            fut.set(parent.first->get_id());
        }
    }
}

void LandmarkStatusManager::progress_reasonable_ordering(
    int id, const BitsetView &past, BitsetView &fut) {
    if (past.test(id)) {
        return;
    }

    for (auto &child : lm_graph.get_node(id)->children) {
        if (child.second == EdgeType::REASONABLE) {
            fut.set(child.first->get_id());
        }
    }
}

void LandmarkStatusManager::update_lm_status(const State &ancestor_state) {
    /*
      TODO: We could get rid of this function by accessing this information from
       outside directly, i.e., checking **for a given state** whether the
       landmark is past or future or both **in that state** (instead of calling
       *get_landmark_status*). This should be possible but I didn't want to do
       too many changes at once before we start experimenting.
    */
    const BitsetView past = get_past_landmarks(ancestor_state);
    const BitsetView fut = get_future_landmarks(ancestor_state);

    const int num_landmarks = lm_graph.get_num_landmarks();
    for (int id = 0; id < num_landmarks; ++id) {
        if (!past.test(id)) {
            assert(fut.test(id));
            lm_status[id] = FUTURE;
        } else if (!fut.test(id)) {
            assert(past.test(id));
            lm_status[id] = PAST;
        } else {
            lm_status[id] = PAST_AND_FUTURE;
        }
    }
}
}
