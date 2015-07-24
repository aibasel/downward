#include "distances.h"

#include "transition_system.h"

#include "../priority_queue.h"

#include <cassert>
#include <deque>

using namespace std;


const int Distances::DISTANCE_UNKNOWN;


Distances::Distances(const TransitionSystem &transition_system)
    : transition_system(transition_system) {
    clear_distances();
}

Distances::~Distances() {
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
    for (const LabelGroup &group : transition_system.get_grouped_labels())
        if (group.get_cost() != 1)
            return false;
    return true;
}

static void breadth_first_search(
    const vector<vector<int> > &graph, deque<int> &queue,
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
    const list<LabelGroup> &grouped_labels =
        transition_system.get_grouped_labels();

    vector<vector<int> > forward_graph(get_num_states());
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = group_it->get_const_transitions();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            forward_graph[transition.src].push_back(transition.target);
        }
    }

    deque<int> queue;
    // TODO: This is an oddly inefficient initialization! Fix it.
    for (int state = 0; state < get_num_states(); ++state) {
        if (state == transition_system.get_init_state()) {
            init_distances[state] = 0;
            queue.push_back(state);
        }
    }
    breadth_first_search(forward_graph, queue, init_distances);
}

void Distances::compute_goal_distances_unit_cost() {
    const list<LabelGroup> &grouped_labels =
        transition_system.get_grouped_labels();

    vector<vector<int> > backward_graph(get_num_states());
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = group_it->get_const_transitions();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
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
    const vector<vector<pair<int, int> > > &graph,
    AdaptiveQueue<int> &queue,
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
    const list<LabelGroup> &grouped_labels =
        transition_system.get_grouped_labels();

    vector<vector<pair<int, int> > > forward_graph(get_num_states());
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = group_it->get_const_transitions();
        int cost = group_it->get_cost();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            forward_graph[transition.src].push_back(
                make_pair(transition.target, cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_goal_distances_general_cost.
    AdaptiveQueue<int> queue;
    // TODO: This is an oddly inefficient initialization! Fix it.
    for (int state = 0; state < get_num_states(); ++state) {
        if (state == transition_system.get_init_state()) {
            init_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(forward_graph, queue, init_distances);
}

void Distances::compute_goal_distances_general_cost() {
    const list<LabelGroup> &grouped_labels =
        transition_system.get_grouped_labels();

    vector<vector<pair<int, int> > > backward_graph(get_num_states());
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = group_it->get_const_transitions();
        int cost = group_it->get_cost();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            backward_graph[transition.target].push_back(
                make_pair(transition.src, cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_init_distances_general_cost.
    AdaptiveQueue<int> queue;
    for (int state = 0; state < get_num_states(); ++state) {
        if (transition_system.is_goal_state(state)) {
            goal_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(backward_graph, queue, goal_distances);
}

void Distances::clear_distances() {
    max_f = DISTANCE_UNKNOWN;
    max_g = DISTANCE_UNKNOWN;
    max_h = DISTANCE_UNKNOWN;
    init_distances.clear();
    goal_distances.clear();
}

bool Distances::are_distances_computed() const {
    if (max_h == DISTANCE_UNKNOWN) {
        assert(max_f == DISTANCE_UNKNOWN);
        assert(max_g == DISTANCE_UNKNOWN);
        assert(init_distances.empty());
        assert(goal_distances.empty());
        return false;
    }
    return true;
}

std::vector<bool> Distances::compute_distances() {
    /*
      This method does the following:
      - Computes the distances of abstract states from the abstract
        initial state ("abstract g") and from the abstract goal states
        ("abstract h").
      - Set max_f, max_g and max_h.
      - Return a vector<bool> that indicates which states can be pruned
        because the are unreachable (abstract g is infinite) or
        irrelevant (abstract h is infinite).
      - Display statistics on max_f, max_g and max_h and on unreachable
        and irrelevant states.
    */

    cout << transition_system.tag() << flush;
    assert(!are_distances_computed());
    assert(init_distances.empty() && goal_distances.empty());

    int num_states = get_num_states();

    if (transition_system.get_init_state() == TransitionSystem::PRUNED_STATE) {
        cout << "init state was pruned, no distances to compute" << endl;
        // If init_state was pruned, then everything must have been pruned.
        assert(num_states == 0);
        max_f = max_g = max_h = INF;
        return vector<bool>();
    }

    init_distances.resize(num_states, INF);
    goal_distances.resize(num_states, INF);
    if (is_unit_cost()) {
        cout << "computing distances using unit-cost algorithm" << endl;
        compute_init_distances_unit_cost();
        compute_goal_distances_unit_cost();
    } else {
        cout << "computing distances using general-cost algorithm" << endl;
        compute_init_distances_general_cost();
        compute_goal_distances_general_cost();
    }

    max_f = 0;
    max_g = 0;
    max_h = 0;

    int unreachable_count = 0, irrelevant_count = 0;
    vector<bool> to_be_pruned_states(num_states, false);
    for (int i = 0; i < num_states; ++i) {
        int g = init_distances[i];
        int h = goal_distances[i];
        // States that are both unreachable and irrelevant are counted
        // as unreachable, not irrelevant. (Doesn't really matter, of
        // course.)
        if (g == INF) {
            ++unreachable_count;
            to_be_pruned_states[i] = true;
        } else if (h == INF) {
            ++irrelevant_count;
            to_be_pruned_states[i] = true;
        } else {
            max_f = max(max_f, g + h);
            max_g = max(max_g, g);
            max_h = max(max_h, h);
        }
    }
    if (unreachable_count || irrelevant_count) {
        cout << transition_system.tag()
             << "unreachable: " << unreachable_count << " states, "
             << "irrelevant: " << irrelevant_count << " states" << endl;
    }
    assert(are_distances_computed());
    return to_be_pruned_states;
}

int Distances::get_max_f() const {
    return max_f;
}

int Distances::get_max_g() const {
    return max_g;
}

int Distances::get_max_h() const {
    return max_h;
}
