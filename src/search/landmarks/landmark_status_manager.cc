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
        vector<LandmarkNode *> greedy_necessary_children;
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
        vector<LandmarkNode *> reasonable_parents;
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
      /* We initialize to true in *past_landmarks* because true is the
         neutral element of conjunction/set intersection. */
      past_landmarks(vector<bool>(graph.get_num_landmarks(), true)),
      /* We initialize to false in *future_landmarks* because false is
         the neutral element for disjunction/set union. */
      future_landmarks(vector<bool>(graph.get_num_landmarks(), false)) {
}

BitsetView LandmarkStatusManager::get_past_landmarks(const State &state) {
    return past_landmarks[state];
}

BitsetView LandmarkStatusManager::get_future_landmarks(const State &state) {
    return future_landmarks[state];
}

ConstBitsetView LandmarkStatusManager::get_past_landmarks(const State &state) const {
    return past_landmarks[state];
}

ConstBitsetView LandmarkStatusManager::get_future_landmarks(const State &state) const {
    return future_landmarks[state];
}

void LandmarkStatusManager::progress_initial_state(const State &initial_state) {
    BitsetView past = get_past_landmarks(initial_state);
    BitsetView future = get_future_landmarks(initial_state);

    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();
        const Landmark &lm = node->get_landmark();
        if (lm.is_true_in_state(initial_state)) {
            assert(past.test(id));
            /*
              A landmark B that holds initially is always past. If there is a
              reasonable ordering A->B such that A does not hold initially, we
              know B must be added again after A is added (or at the same time).
              Therefore, we can set B future in these cases.

              In solvable tasks and for natural (or stronger) orderings A->B, it
              is not valid for B to hold initially. Hence, if such an ordering
              exists, the problem is unsolvable. Consequently, it is fine to
              mark B future also in these cases, because for unsolvable
              problems anything is a landmark.
            */
            if (any_of(node->parents.begin(), node->parents.end(),
                       [initial_state](auto &parent) {
                           Landmark &landmark = parent.first->get_landmark();
                           return !landmark.is_true_in_state(initial_state);
                       })) {
                future.set(id);
            }
        } else {
            past.reset(id);
            future.set(id);
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

    ConstBitsetView parent_past = get_past_landmarks(parent_ancestor_state);
    BitsetView past = get_past_landmarks(ancestor_state);

    ConstBitsetView parent_future = get_future_landmarks(parent_ancestor_state);
    BitsetView future = get_future_landmarks(ancestor_state);

    assert(past.size() == lm_graph.get_num_landmarks());
    assert(parent_past.size() == lm_graph.get_num_landmarks());
    assert(future.size() == lm_graph.get_num_landmarks());
    assert(parent_future.size() == lm_graph.get_num_landmarks());

    progress_landmarks(
        parent_past, parent_future, parent_ancestor_state,
        past, future, ancestor_state);
    progress_goals(ancestor_state, future);
    progress_greedy_necessary_orderings(ancestor_state, past, future);
    progress_reasonable_orderings(past, future);
}

void LandmarkStatusManager::progress_landmarks(
    ConstBitsetView &parent_past, ConstBitsetView &parent_future,
    const State &parent_ancestor_state, BitsetView &past,
    BitsetView &future, const State &ancestor_state) {
    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();
        const Landmark &lm = node->get_landmark();
        if (parent_future.test(id)) {
            if (!lm.is_true_in_state(ancestor_state)) {
                /*
                  A landmark that is future in the parent remains future
                  if it does not hold in the current state. If it also
                  wasn't past in the parent, it remains not past.
                */
                future.set(id);
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
                future.set(id);
            }
        }
    }
}

void LandmarkStatusManager::progress_goals(const State &ancestor_state,
                                           BitsetView &future) {
    for (auto &node : goal_landmarks) {
        if (!node->get_landmark().is_true_in_state(ancestor_state)) {
            future.set(node->get_id());
        }
    }
}

void LandmarkStatusManager::progress_greedy_necessary_orderings(
    const State &ancestor_state, const BitsetView &past, BitsetView &future) {
    for (auto &[tail, children] : greedy_necessary_children) {
        const Landmark &lm = tail->get_landmark();
        assert(!children.empty());
        for (auto &child : children) {
            if (!past.test(child->get_id())
                && !lm.is_true_in_state(ancestor_state)) {
                future.set(tail->get_id());
                break;
            }
        }
    }
}

void LandmarkStatusManager::progress_reasonable_orderings(
    const BitsetView &past, BitsetView &future) {
    for (auto &[head, parents] : reasonable_parents) {
        assert(!parents.empty());
        for (auto &parent : parents) {
            if (!past.test(parent->get_id())) {
                future.set(head->get_id());
                break;
            }
        }
    }
}
}
