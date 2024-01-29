#include "cg_heuristic.h"

#include "cg_cache.h"
#include "domain_transition_graph.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

using namespace std;
using namespace domain_transition_graph;

namespace cg_heuristic {
CGHeuristic::CGHeuristic(const plugins::Options &opts)
    : Heuristic(opts),
      cache_hits(0),
      cache_misses(0),
      helpful_transition_extraction_counter(0),
      min_action_cost(task_properties::get_min_operator_cost(task_proxy)) {
    if (log.is_at_least_normal()) {
        log << "Initializing causal graph heuristic..." << endl;
    }

    int max_cache_size = opts.get<int>("max_cache_size");
    if (max_cache_size > 0)
        cache = utils::make_unique_ptr<CGCache>(task_proxy, max_cache_size, log);

    unsigned int num_vars = task_proxy.get_variables().size();
    prio_queues.reserve(num_vars);
    for (size_t i = 0; i < num_vars; ++i)
        prio_queues.push_back(utils::make_unique_ptr<ValueNodeQueue>());

    function<bool(int, int)> pruning_condition =
        [](int dtg_var, int cond_var) {return dtg_var <= cond_var;};
    DTGFactory factory(task_proxy, false, pruning_condition);
    transition_graphs = factory.build_dtgs();
}

CGHeuristic::~CGHeuristic() {
}

bool CGHeuristic::dead_ends_are_reliable() const {
    return false;
}

int CGHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    setup_domain_transition_graphs();

    int heuristic = 0;
    for (FactProxy goal : task_proxy.get_goals()) {
        const VariableProxy var = goal.get_variable();
        int var_no = var.get_id();
        int from = state[var_no].get_value(), to = goal.get_value();
        DomainTransitionGraph *dtg = transition_graphs[var_no].get();
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
    for (auto &dtg : transition_graphs) {
        for (auto &node : dtg->nodes) {
            node.distances.clear();
            node.helpful_transitions.clear();
        }
    }
    // Reset "dirty bits" for helpful transitions.
    ++helpful_transition_extraction_counter;
}

int CGHeuristic::get_transition_cost(const State &state,
                                     DomainTransitionGraph *dtg,
                                     int start_val,
                                     int goal_val) {
    if (start_val == goal_val)
        return 0;

    int var_no = dtg->var;

    // Check cache.
    bool use_the_cache = cache && cache->is_cached(var_no);
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
        start->helpful_transitions.resize(dtg->nodes.size(), nullptr);
        start->distances[start_val] = 0;
        start->reached_from = nullptr;
        start->reached_by = nullptr;
        start->children_state.resize(dtg->local_to_global_child.size());
        for (size_t i = 0; i < dtg->local_to_global_child.size(); ++i) {
            start->children_state[i] =
                state[dtg->local_to_global_child[i]].get_value();
        }

        // Initialize Heap for Dijkstra's algorithm.
        priority_queues::AdaptiveQueue<ValueNode *> &prio_queue = *prio_queues[var_no];
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
                for (const LocalAssignment &assign : precond)
                    source->children_state[assign.local_var] = assign.value;
            }

            // Scan outgoing transitions.
            for (ValueTransition &transition : source->transitions) {
                ValueNode *target = transition.target;
                int *target_distance_ptr = &start->distances[target->value];

                // Scan labels of the transition.
                for (ValueTransitionLabel &label : transition.labels) {
                    OperatorProxy op = label.is_axiom ?
                        task_proxy.get_axioms()[label.op_id] :
                        task_proxy.get_operators()[label.op_id];
                    int new_distance = source_distance + op.get_cost();
                    for (LocalAssignment &assignment : label.precond) {
                        if (new_distance >= *target_distance_ptr)
                            break;  // We already know this isn't an improved path.
                        int local_var = assignment.local_var;
                        int current_val = source->children_state[local_var];
                        int global_var = dtg->local_to_global_child[local_var];
                        DomainTransitionGraph *precond_dtg =
                            transition_graphs[global_var].get();
                        int recursive_cost = get_transition_cost(
                            state, precond_dtg, current_val, assignment.value);
                        if (recursive_cost == numeric_limits<int>::max())
                            new_distance = numeric_limits<int>::max();
                        else
                            new_distance += recursive_cost;
                    }

                    if (new_distance < min_action_cost) {
                        /*
                          If the cost is lower than the min action
                          cost, we know we're too optimistic, so we
                          might as well increase it. This helps quite
                          a bit in PSR-Large, apparently, which is why
                          this is in, but this should probably an
                          option.

                          TODO: Evaluate impact of this.
                        */
                        new_distance = min_action_cost;
                    }

                    if (*target_distance_ptr > new_distance) {
                        // Update node in heap and update its internal state.
                        *target_distance_ptr = new_distance;
                        target->reached_from = source;
                        target->reached_by = &label;

                        if (current_helpful_transition == nullptr) {
                            // This transition starts at the start node;
                            // no helpful transitions recorded yet.
                            start->helpful_transitions[target->value] = &label;
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
        int num_values = start->distances.size();
        for (int val = 0; val < num_values; ++val) {
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
    int from = state[var_no].get_value();
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
    if (cache && cache->is_cached(var_no)) {
        helpful = cache->lookup_helpful_transition(var_no, state, from, to);
        cost = cache->lookup(var_no, state, from, to);
        assert(helpful);
    } else {
        ValueNode *start_node = &dtg->nodes[from];
        assert(!start_node->helpful_transitions.empty());
        helpful = start_node->helpful_transitions[to];
        cost = start_node->distances[to];
    }

    OperatorProxy op = helpful->is_axiom ?
        task_proxy.get_axioms()[helpful->op_id] :
        task_proxy.get_operators()[helpful->op_id];
    if (cost == op.get_cost() &&
        !op.is_axiom() &&
        task_properties::is_applicable(op, state)) {
        // Transition immediately applicable, all preconditions true.
        set_preferred(op);
    } else {
        // Recursively compute helpful transitions for the precondition variables.
        for (const LocalAssignment &assignment : helpful->precond) {
            int local_var = assignment.local_var;
            int global_var = dtg->local_to_global_child[local_var];
            DomainTransitionGraph *precond_dtg = transition_graphs[global_var].get();
            mark_helpful_transitions(state, precond_dtg, assignment.value);
        }
    }
}

class CGHeuristicFeature : public plugins::TypedFeature<Evaluator, CGHeuristic> {
public:
    CGHeuristicFeature() : TypedFeature("cg") {
        document_title("Causal graph heuristic");

        add_option<int>(
            "max_cache_size",
            "maximum number of cached entries per variable (set to 0 to disable cache)",
            "1000000",
            plugins::Bounds("0", "infinity"));
        Heuristic::add_options_to_feature(*this);

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "supported");
        document_language_support(
            "axioms",
            "supported (in the sense that the planner won't complain -- "
            "handling of axioms might be very stupid "
            "and even render the heuristic unsafe)");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "no");
        document_property("preferred operators", "yes");
    }
};

static plugins::FeaturePlugin<CGHeuristicFeature> _plugin;
}
