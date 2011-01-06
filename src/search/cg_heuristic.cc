#include "cg_heuristic.h"

#include "cg_cache.h"
#include "domain_transition_graph.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>
using namespace std;

// TODO: Turn this into an option and check its impact.
#define USE_CACHE true


static ScalarEvaluatorPlugin cg_heuristic_plugin("cg", CGHeuristic::create);


CGHeuristic::CGHeuristic(const HeuristicOptions &options)
    : Heuristic(options),
      cache(new CGCache), cache_hits(0), cache_misses(0),
      helpful_transition_extraction_counter(0) {
    prio_queues.reserve(g_transition_graphs.size());
    for (int i = 0; i < g_transition_graphs.size(); ++i)
        prio_queues.push_back(new AdaptiveQueue<ValueNode *>);
}

CGHeuristic::~CGHeuristic() {
    for (int i = 0; i < prio_queues.size(); ++i)
        delete prio_queues[i];
}

void CGHeuristic::initialize() {
    cout << "Initializing causal graph heuristic..." << endl;
}

int CGHeuristic::compute_heuristic(const State &state) {
    setup_domain_transition_graphs();

    int heuristic = 0;
    for (int i = 0; i < g_goal.size(); i++) {
        int var_no = g_goal[i].first, from = state[var_no], to = g_goal[i].second;
        DomainTransitionGraph *dtg = g_transition_graphs[var_no];
        int cost_for_goal = get_transition_cost(state, dtg, from, to);
        if (cost_for_goal == numeric_limits<int>::max()) {
            return DEAD_END;
        } else {
            heuristic += cost_for_goal;
            mark_helpful_transitions(state, dtg, to);
        }
    }
    return heuristic;
}

void CGHeuristic::setup_domain_transition_graphs() {
    for (int var = 0; var < g_transition_graphs.size(); var++) {
        DomainTransitionGraph *dtg = g_transition_graphs[var];
        for (int i = 0; i < dtg->nodes.size(); i++) {
            dtg->nodes[i].distances.clear();
            dtg->nodes[i].helpful_transitions.clear();
        }
    }
    // Reset "dirty bits" for helpful transitions.
    helpful_transition_extraction_counter++;
}

int CGHeuristic::get_transition_cost(const State &state,
                                     DomainTransitionGraph *dtg, int start_val,
                                     int goal_val) {
    if (start_val == goal_val)
        return 0;

    int var_no = dtg->var;

    // Check cache.
    bool use_the_cache = USE_CACHE && cache->is_cached(var_no);
    if (use_the_cache) {
        int cached_val = cache->lookup(var_no, state, start_val, goal_val);
        if (cached_val != CGCache::NOT_COMPUTED) {
            ++cache_hits;
            return cached_val;
        }
    } else {
        ++cache_misses;
    }

    ValueNode *start = &dtg->nodes[start_val];
    if (start->distances.empty()) {
        // Initialize data of initial node.
        start->distances.resize(dtg->nodes.size(), numeric_limits<int>::max());
        start->helpful_transitions.resize(dtg->nodes.size(), 0);
        start->distances[start_val] = 0;
        start->reached_from = 0;
        start->reached_by = 0;
        start->children_state.resize(dtg->local_to_global_child.size());
        for (int i = 0; i < dtg->local_to_global_child.size(); i++)
            start->children_state[i] = state[dtg->local_to_global_child[i]];

        // Initialize Heap for Dijkstra's algorithm.
        AdaptiveQueue<ValueNode *> &prio_queue = *prio_queues[var_no];
        prio_queue.clear();
        prio_queue.push(0, start);

        // Dijkstra algorithm main loop.
        while (!prio_queue.empty()) {
            pair<int, ValueNode *> top_pair = prio_queue.pop();
            int source_distance = top_pair.first;
            ValueNode *source = top_pair.second;

            assert(start->distances[source->value] <= source_distance);
            if (start->distances[source->value] < source_distance)
                continue;

            ValueTransitionLabel *current_helpful_transition =
                start->helpful_transitions[source->value];

            // Set children state for all nodes but the initial.
            if (source->value != start_val) {
                source->children_state = source->reached_from->children_state;
                vector<LocalAssignment> &precond = source->reached_by->precond;
                for (int k = 0; k < precond.size(); k++)
                    source->children_state[precond[k].local_var] = precond[k].value;
            }

            // Scan outgoing transitions.
            for (int i = 0; i < source->transitions.size(); i++) {
                ValueTransition *transition = &source->transitions[i];
                ValueNode *target = transition->target;
                int *target_distance_ptr = &start->distances[target->value];

                // Scan labels of the transition.
                for (int j = 0; j < transition->labels.size(); j++) {
                    ValueTransitionLabel *label = &transition->labels[j];
                    int new_distance = source_distance + get_adjusted_cost(*label->op);
                    vector<LocalAssignment> &precond = label->precond;
                    for (int k = 0; k < precond.size(); k++) {
                        if (new_distance >= *target_distance_ptr)
                            break;  // We already know this isn't an improved path.
                        int local_var = precond[k].local_var;
                        int current_val = source->children_state[local_var];
                        int global_var = dtg->local_to_global_child[local_var];
                        DomainTransitionGraph *precond_dtg =
                            g_transition_graphs[global_var];
                        int recursive_cost = get_transition_cost(
                            state, precond_dtg, current_val, precond[k].value);
                        if (recursive_cost == numeric_limits<int>::max())
                            new_distance = numeric_limits<int>::max();
                        else
                            new_distance += recursive_cost;
                    }

                    if (new_distance < g_min_action_cost) {
                        /*
                          If the cost is lower than the min action
                          cost, we know we're too optimistic, so we
                          might as well increase it. This helps quite
                          a bit in PSR-Large, apparently, which is why
                          this is in, but this should probably an
                          option.

                          TODO: Evaluate impact of this.
                        */
                        new_distance = g_min_action_cost;
                    }

                    if (*target_distance_ptr > new_distance) {
                        // Update node in heap and update its internal state.
                        *target_distance_ptr = new_distance;
                        target->reached_from = source;
                        target->reached_by = label;

                        if (current_helpful_transition == 0) {
                            // This transition starts at the start node;
                            // no helpful transitions recorded yet.
                            start->helpful_transitions[target->value] = label;
                        } else {
                            start->helpful_transitions[target->value] = current_helpful_transition;
                        }

                        prio_queue.push(new_distance, target);
                    }
                }
            }
        }
    }

    if (use_the_cache) {
        for (int val = 0; val < start->distances.size(); val++) {
            if (val == start_val)
                continue;
            int distance = start->distances[val];
            ValueTransitionLabel *helpful = start->helpful_transitions[val];
            // We should have a helpful transition iff distance is infinite.
            assert((distance == numeric_limits<int>::max()) == !helpful);
            cache->store(var_no, state, start_val, val, distance);
            cache->store_helpful_transition(
                var_no, state, start_val, val, helpful);
        }
    }

    return start->distances[goal_val];
}

void CGHeuristic::mark_helpful_transitions(const State &state,
                                           DomainTransitionGraph *dtg, int to) {
    int var_no = dtg->var;
    int from = state[var_no];
    if (from == to)
        return;

    /*
      Avoid checking helpful transitions for the same variable twice
      via different paths of recursion.

      Interestingly, this technique even blocks further calls with the
      same variable *if the to value is different*. This looks wrong,
      but in first, very preliminary tests, this appeared better in
      terms of evaluations than not blocking such calls. Maybe it's
      better to pick only a few preferred operators since this focuses
      search more?

      TODO: Test this more systematically. An easy way to test this is
      by simply removing the following test-and-return. Of course,
      this also has a performance impact, so the correct way to test
      this is by looking at evaluations/expansions only. If it turns
      out that this is an interesting choice, we should look into this
      more deeply and maybe turn this into an option.
     */
    if (dtg->last_helpful_transition_extraction_time ==
        helpful_transition_extraction_counter)
        return;
    dtg->last_helpful_transition_extraction_time =
        helpful_transition_extraction_counter;

    ValueTransitionLabel *helpful;
    int cost;
    // Check cache.
    if (USE_CACHE && cache->is_cached(var_no)) {
        helpful = cache->lookup_helpful_transition(var_no, state, from, to);
        cost = cache->lookup(var_no, state, from, to);
        assert(helpful);
    } else {
        ValueNode *start_node = &dtg->nodes[from];
        assert(!start_node->helpful_transitions.empty());
        helpful = start_node->helpful_transitions[to];
        cost = start_node->distances[to];
    }

    if (cost == get_adjusted_cost(*helpful->op) && !helpful->op->is_axiom()
        && helpful->op->is_applicable(state)) {
        // Transition immediately applicable, all preconditions true.
        set_preferred(helpful->op);
    } else {
        // Recursively compute helpful transitions for the precondition variables.
        const vector<LocalAssignment> &precond = helpful->precond;
        for (int i = 0; i < precond.size(); i++) {
            int local_var = precond[i].local_var;
            int global_var = dtg->local_to_global_child[local_var];
            DomainTransitionGraph *precond_dtg = g_transition_graphs[global_var];
            mark_helpful_transitions(state, precond_dtg, precond[i].value);
        }
    }
}

ScalarEvaluator *CGHeuristic::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    HeuristicOptions common_options;

    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            common_options.add_option_to_parser(option_parser);

            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    if (dry_run) {
        return 0;
    } else {
        return new CGHeuristic(common_options);
    }
}
