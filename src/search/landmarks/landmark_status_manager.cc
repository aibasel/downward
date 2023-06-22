#include "landmark_status_manager.h"
#include "util.h"

#include "landmark.h"

using namespace std;

namespace landmarks {
static vector<LandmarkNode *> get_goal_landmarks(const LandmarkGraph &graph) {
    vector<LandmarkNode *> goals;
    for (auto &node : graph.get_nodes()) {
        if (node->get_landmark().is_true_in_goal) {
            goals.push_back(node.get());
        }
    }
    return goals;
}

static vector<pair<LandmarkNode *, vector<LandmarkNode *>>> get_greedy_necessary_children(
    const LandmarkGraph &graph) {
    vector<pair<LandmarkNode *, vector<LandmarkNode *>>> orderings;
    for (auto &node : graph.get_nodes()) {
        vector<LandmarkNode *> greedy_necessary_children{};
        for (auto &child : node->children) {
            if (child.second == EdgeType::GREEDY_NECESSARY) {
                greedy_necessary_children.push_back(child.first);
            }
        }
        if (!greedy_necessary_children.empty()) {
            orderings.emplace_back(node.get(), move(greedy_necessary_children));
        }
    }
    return orderings;
}

static vector<pair<LandmarkNode *, vector<LandmarkNode *>>> get_reasonable_parents(
    const LandmarkGraph &graph) {
    vector<pair<LandmarkNode *, vector<LandmarkNode *>>> orderings;
    for (auto &node : graph.get_nodes()) {
        vector<LandmarkNode *> reasonable_parents{};
        for (auto &parent : node->parents) {
            if (parent.second == EdgeType::REASONABLE) {
                reasonable_parents.push_back(parent.first);
            }
        }
        if (!reasonable_parents.empty()) {
            orderings.emplace_back(node.get(), move(reasonable_parents));
        }
    }
    return orderings;
}

/*
  By default we mark all landmarks past, since it is the neutral element for
  set intersection which we emulate in the progression. Similarly, no landmarks
  are future which is the neutral element for set union.
*/
LandmarkStatusManager::LandmarkStatusManager(
    LandmarkGraph &graph,
    bool progress_goals,
    bool progress_greedy_necessary_orderings,
    bool progress_reasonable_orderings)
    : lm_graph(graph),
      goal_landmarks(progress_goals ? get_goal_landmarks(graph)
                     : vector<LandmarkNode *>{}),
      greedy_necessary_children(
          progress_greedy_necessary_orderings
          ? get_greedy_necessary_children(graph)
          : vector<pair<LandmarkNode *, vector<LandmarkNode *>>>{}),
      reasonable_parents(
          progress_reasonable_orderings
          ? get_reasonable_parents(graph)
          : vector<pair<LandmarkNode *, vector<LandmarkNode *>>>{}),
      past_landmarks(vector<bool>(graph.get_num_landmarks(), true)),
      future_landmarks(vector<bool>(graph.get_num_landmarks(), false)) {
}

BitsetView LandmarkStatusManager::get_past_landmarks(const State &state) {
    return past_landmarks[state];
}

BitsetView LandmarkStatusManager::get_future_landmarks(const State &state) {
    return future_landmarks[state];
}

ConstBitsetView LandmarkStatusManager::get_past_landmarks_const(const State &state) const {
    return past_landmarks[state];
}

ConstBitsetView LandmarkStatusManager::get_future_landmarks_const(const State &state) const {
    return future_landmarks[state];
}

void LandmarkStatusManager::progress_initial_state(const State &initial_state) {
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
}

void LandmarkStatusManager::progress(
    const State &parent_ancestor_state, OperatorID,
    const State &ancestor_state) {
    if (ancestor_state == parent_ancestor_state) {
        // This can happen, e.g., in Satellite-01.
        return;
    }

    ConstBitsetView parent_past =
        get_past_landmarks_const(parent_ancestor_state);
    BitsetView past = get_past_landmarks(ancestor_state);

    ConstBitsetView parent_fut =
        get_future_landmarks_const(parent_ancestor_state);
    BitsetView fut = get_future_landmarks(ancestor_state);

    assert(past.size() == lm_graph.get_num_landmarks());
    assert(parent_past.size() == lm_graph.get_num_landmarks());
    assert(fut.size() == lm_graph.get_num_landmarks());
    assert(parent_fut.size() == lm_graph.get_num_landmarks());

    progress_basic(
        parent_past, parent_fut, parent_ancestor_state,
        past, fut, ancestor_state);
    progress_goals(ancestor_state, fut);
    progress_greedy_necessary_orderings(ancestor_state, past, fut);
    progress_reasonable_orderings(past, fut);
}

void LandmarkStatusManager::progress_basic(
    ConstBitsetView &parent_past, ConstBitsetView &parent_fut,
    const State &parent_ancestor_state, BitsetView &past,
    BitsetView &fut, const State &ancestor_state) {
    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();
        const Landmark &lm = node->get_landmark();
        if (parent_fut.test(id)) {
            if (!lm.is_true_in_state(ancestor_state)) {
                /*
                  A landmark that is future in the parent remains future
                  if it does not hold in the current state. If it also
                  wasn't past in the parent, it remains not past.
                */
                fut.set(id);
                if (!parent_past.test(id)) {
                    past.reset(id);
                }
            } else if (lm.is_true_in_state(parent_ancestor_state)) {
                /*
                  If the landmark held in the parent already, then it
                  was not added by this transition and should remain
                  future.
                */
                assert(parent_past.test(id));
                fut.set(id);
            }
        }
    }
}

void LandmarkStatusManager::progress_goals(const State &ancestor_state,
                                           BitsetView &fut) {
    for (auto &node : goal_landmarks) {
        if (!node->get_landmark().is_true_in_state(ancestor_state)) {
            fut.set(node->get_id());
        }
    }
}

void LandmarkStatusManager::progress_greedy_necessary_orderings(
    const State &ancestor_state, const BitsetView &past, BitsetView &fut) {
    for (auto &[tail, children] : greedy_necessary_children) {
        const Landmark &lm = tail->get_landmark();
        assert(!children.empty());
        for (auto &child : children) {
            if (!past.test(child->get_id())
                && !lm.is_true_in_state(ancestor_state)) {
                fut.set(tail->get_id());
                break;
            }
        }
    }
}

void LandmarkStatusManager::progress_reasonable_orderings(
    const BitsetView &past, BitsetView &fut) {
    for (auto &[head, parents] : reasonable_parents) {
        assert(!parents.empty());
        for (auto &parent : parents) {
            if (!past.test(parent->get_id())) {
                fut.set(head->get_id());
                break;
            }
        }
    }
}
}
