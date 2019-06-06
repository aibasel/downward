#include "distances.h"

#include "label_equivalence_relation.h"
#include "transition_system.h"

#include "../algorithms/priority_queues.h"

#include <cassert>
#include <deque>

using namespace std;

namespace merge_and_shrink {
const int Distances::DISTANCE_UNKNOWN;

Distances::Distances(const TransitionSystem &transition_system)
    : transition_system(transition_system) {
    clear_distances();
}

void Distances::clear_distances() {
    init_distances_computed = false;
    goal_distances_computed = false;
    init_distances.clear();
    goal_distances.clear();
}

int Distances::get_num_states() const {
    return transition_system.get_size();
}

bool Distances::is_unit_cost() const {
    /*
      TODO: Is this a good implementation? It differs from the
      previous implementation in transition_system.cc because that
      would require access to more attributes. One nice thing about it
      is that it gets at the label cost information in the same way
      that the actual shortest-path algorithms (e.g.
      compute_goal_distances_general_cost) do.
    */
    for (const GroupAndTransitions &gat : transition_system) {
        const LabelGroup &label_group = gat.label_group;
        if (label_group.get_cost() != 1)
            return false;
    }
    return true;
}

static void breadth_first_search(
    const vector<vector<int>> &graph, deque<int> &queue,
    vector<int> &distances) {
    while (!queue.empty()) {
        int state = queue.front();
        queue.pop_front();
        for (size_t i = 0; i < graph[state].size(); ++i) {
            int successor = graph[state][i];
            if (distances[successor] > distances[state] + 1) {
                distances[successor] = distances[state] + 1;
                queue.push_back(successor);
            }
        }
    }
}

void Distances::compute_init_distances_unit_cost() {
    vector<vector<int>> forward_graph(get_num_states());
    for (const GroupAndTransitions &gat : transition_system) {
        const vector<Transition> &transitions = gat.transitions;
        for (const Transition &transition : transitions) {
            forward_graph[transition.src].push_back(transition.target);
        }
    }

    deque<int> queue;
    queue.push_back(transition_system.get_init_state());
    init_distances[transition_system.get_init_state()] = 0;
    breadth_first_search(forward_graph, queue, init_distances);
}

void Distances::compute_goal_distances_unit_cost() {
    vector<vector<int>> backward_graph(get_num_states());
    for (const GroupAndTransitions &gat : transition_system) {
        const vector<Transition> &transitions = gat.transitions;
        for (const Transition &transition : transitions) {
            backward_graph[transition.target].push_back(transition.src);
        }
    }

    deque<int> queue;
    for (int state = 0; state < get_num_states(); ++state) {
        if (transition_system.is_goal_state(state)) {
            goal_distances[state] = 0;
            queue.push_back(state);
        }
    }
    breadth_first_search(backward_graph, queue, goal_distances);
}

static void dijkstra_search(
    const vector<vector<pair<int, int>>> &graph,
    priority_queues::AdaptiveQueue<int> &queue,
    vector<int> &distances) {
    while (!queue.empty()) {
        pair<int, int> top_pair = queue.pop();
        int distance = top_pair.first;
        int state = top_pair.second;
        int state_distance = distances[state];
        assert(state_distance <= distance);
        if (state_distance < distance)
            continue;
        for (size_t i = 0; i < graph[state].size(); ++i) {
            const pair<int, int> &transition = graph[state][i];
            int successor = transition.first;
            int cost = transition.second;
            int successor_cost = state_distance + cost;
            if (distances[successor] > successor_cost) {
                distances[successor] = successor_cost;
                queue.push(successor_cost, successor);
            }
        }
    }
}

void Distances::compute_init_distances_general_cost() {
    vector<vector<pair<int, int>>> forward_graph(get_num_states());
    for (const GroupAndTransitions &gat : transition_system) {
        const LabelGroup &label_group = gat.label_group;
        const vector<Transition> &transitions = gat.transitions;
        int cost = label_group.get_cost();
        for (const Transition &transition : transitions) {
            forward_graph[transition.src].push_back(
                make_pair(transition.target, cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_goal_distances_general_cost.
    priority_queues::AdaptiveQueue<int> queue;
    init_distances[transition_system.get_init_state()] = 0;
    queue.push(0, transition_system.get_init_state());
    dijkstra_search(forward_graph, queue, init_distances);
}

void Distances::compute_goal_distances_general_cost() {
    vector<vector<pair<int, int>>> backward_graph(get_num_states());
    for (const GroupAndTransitions &gat : transition_system) {
        const LabelGroup &label_group = gat.label_group;
        const vector<Transition> &transitions = gat.transitions;
        int cost = label_group.get_cost();
        for (const Transition &transition : transitions) {
            backward_graph[transition.target].push_back(
                make_pair(transition.src, cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_init_distances_general_cost.
    priority_queues::AdaptiveQueue<int> queue;
    for (int state = 0; state < get_num_states(); ++state) {
        if (transition_system.is_goal_state(state)) {
            goal_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(backward_graph, queue, goal_distances);
}

void Distances::compute_distances(
    bool compute_init_distances,
    bool compute_goal_distances,
    Verbosity verbosity) {
    assert(compute_init_distances || compute_goal_distances);
    /*
      This method does the following:
      - Computes the distances of abstract states from the abstract
        initial state ("abstract g") and to the abstract goal states
        ("abstract h"), depending on the given flags.
    */

    if (are_init_distances_computed()) {
        /*
          The only scenario where distance information is allowed to be
          present when computing distances is when computing goal distances
          for the final transition system in a setting where only init
          distances have been computed during the merge-and-shrink computation.
        */
        assert(!are_goal_distances_computed());
        assert(goal_distances.empty());
        assert(!compute_init_distances);
        assert(compute_goal_distances);
    } else {
        /*
          Otherwise, when computing distances, the previous (invalid)
          distance information must have been cleared before.
        */
        assert(!are_init_distances_computed() && !are_goal_distances_computed());
        assert(init_distances.empty() && goal_distances.empty());
    }

    if (verbosity >= Verbosity::VERBOSE) {
        cout << transition_system.tag();
    }

    int num_states = get_num_states();
    if (num_states == 0) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << "empty transition system, no distances to compute" << endl;
        }
        init_distances_computed = true;
        goal_distances_computed = true;
        return;
    }

    if (compute_init_distances) {
        init_distances.resize(num_states, INF);
    }
    if (compute_goal_distances) {
        goal_distances.resize(num_states, INF);
    }
    if (verbosity >= Verbosity::VERBOSE) {
        cout << "computing ";
        if (compute_init_distances && compute_goal_distances) {
            cout << "init and goal";
        } else if (compute_init_distances) {
            cout << "init";
        } else if (compute_goal_distances) {
            cout << "goal";
        }
        cout << " distances using ";
    }
    if (is_unit_cost()) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << "unit-cost";
        }
        if (compute_init_distances) {
            compute_init_distances_unit_cost();
        }
        if (compute_goal_distances) {
            compute_goal_distances_unit_cost();
        }
    } else {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << "general-cost";
        }
        if (compute_init_distances) {
            compute_init_distances_general_cost();
        }
        if (compute_goal_distances) {
            compute_goal_distances_general_cost();
        }
    }
    if (verbosity >= Verbosity::VERBOSE) {
        cout << " algorithm" << endl;
    }

    if (compute_init_distances) {
        init_distances_computed = true;
    }
    if (compute_goal_distances) {
        goal_distances_computed = true;
    }
}

void Distances::apply_abstraction(
    const StateEquivalenceRelation &state_equivalence_relation,
    bool compute_init_distances,
    bool compute_goal_distances,
    Verbosity verbosity) {
    if (compute_init_distances) {
        assert(are_init_distances_computed());
        assert(state_equivalence_relation.size() < init_distances.size());
    }
    if (compute_goal_distances) {
        assert(are_goal_distances_computed());
        assert(state_equivalence_relation.size() < goal_distances.size());
    }

    int new_num_states = state_equivalence_relation.size();
    vector<int> new_init_distances;
    vector<int> new_goal_distances;
    if (compute_init_distances) {
        new_init_distances.resize(new_num_states, DISTANCE_UNKNOWN);
    }
    if (compute_goal_distances) {
        new_goal_distances.resize(new_num_states, DISTANCE_UNKNOWN);
    }

    bool must_recompute = false;
    for (int new_state = 0; new_state < new_num_states; ++new_state) {
        const StateEquivalenceClass &state_equivalence_class =
            state_equivalence_relation[new_state];
        assert(!state_equivalence_class.empty());

        StateEquivalenceClass::const_iterator pos = state_equivalence_class.begin();
        int new_init_dist = -1;
        int new_goal_dist = -1;
        if (compute_init_distances) {
            new_init_dist = init_distances[*pos];
        }
        if (compute_goal_distances) {
            new_goal_dist = goal_distances[*pos];
        }

        ++pos;
        for (; pos != state_equivalence_class.end(); ++pos) {
            if (compute_init_distances && init_distances[*pos] != new_init_dist) {
                must_recompute = true;
                break;
            }
            if (compute_goal_distances && goal_distances[*pos] != new_goal_dist) {
                must_recompute = true;
                break;
            }
        }

        if (must_recompute)
            break;

        if (compute_init_distances) {
            new_init_distances[new_state] = new_init_dist;
        }
        if (compute_goal_distances) {
            new_goal_distances[new_state] = new_goal_dist;
        }
    }

    if (must_recompute) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << transition_system.tag()
                 << "simplification was not f-preserving!" << endl;
        }
        clear_distances();
        compute_distances(
            compute_init_distances, compute_goal_distances, verbosity);
    } else {
        init_distances = move(new_init_distances);
        goal_distances = move(new_goal_distances);
    }
}

void Distances::dump() const {
    cout << "Distances: ";
    for (size_t i = 0; i < goal_distances.size(); ++i) {
        cout << i << ": " << goal_distances[i] << ", ";
    }
    cout << endl;
}

void Distances::statistics() const {
    cout << transition_system.tag();
    if (!are_goal_distances_computed()) {
        cout << "goal distances not computed";
    } else if (transition_system.is_solvable(*this)) {
        cout << "init h=" << get_goal_distance(transition_system.get_init_state());
    } else {
        cout << "transition system is unsolvable";
    }
    cout << endl;
}
}
