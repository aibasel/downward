#include "cea_heuristic.h"

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

/*
TODO: The responsibilities between the different classes need to be
      divided more clearly. For example, the LocalProblem
      initialization code contains some stuff (related to initial node
      initialization) that may better fit into the LocalProblemNode
      class. It's probably best to have all the code in the
      ContextEnhancedAdditiveHeuristic class and have everything else be PODs.
      This would also get rid of g_HACK.
 */


static ScalarEvaluatorPlugin context_enhanced_additive_heuristic_plugin(
    "cea", ContextEnhancedAdditiveHeuristic::create);


static ContextEnhancedAdditiveHeuristic *g_HACK = 0;

inline void ContextEnhancedAdditiveHeuristic::add_to_heap(
    LocalProblemNode *node) {
    node_queue.push(node->priority(), node);
}

LocalTransition::LocalTransition(
    LocalProblemNode *source_, LocalProblemNode *target_,
    const ValueTransitionLabel *label_, int action_cost_) {
    source = source_;
    target = target_;
    label = label_;
    action_cost = action_cost_;

    // Set the following to explicitly invalid values.
    // They are initialized in on_source_expanded.
    target_cost = -1;
    unreached_conditions = -1;
}

inline void LocalTransition::try_to_fire() {
    if (!unreached_conditions && target_cost < target->cost) {
        target->cost = target_cost;
        target->reached_by = this;
        g_HACK->add_to_heap(target);
    }
}

void LocalTransition::on_source_expanded(const State &state) {
    /* Called when the source of this transition is reached by
       Dijkstra exploration. Tries to compute cost for the target of
       the transition from the source cost, action cost, and set-up
       costs for the conditions on the label. The latter may yet be
       unknown, in which case we "subscribe" to the waiting list of
       the node that will tell us the correct value. */

    assert(source->cost >= 0);
    assert(source->cost < numeric_limits<int>::max());

    target_cost = source->cost + action_cost;

    if (target->cost <= target_cost) {
        // Transition cannot find a shorter path to target.
        return;
    }

    unreached_conditions = 0;
    const vector<LocalAssignment> &precond = label->precond;

    vector<LocalAssignment>::const_iterator
        curr_precond = precond.begin(),
        last_precond = precond.end();

    short *children_state = &source->children_state.front();
    vector<vector<LocalProblem *> > &problem_index = g_HACK->local_problem_index;
    int *parent_vars = &*source->owner->causal_graph_parents->begin();

    for (; curr_precond != last_precond; ++curr_precond) {
        int local_var = curr_precond->local_var;
        int current_val = children_state[local_var];
        int precond_value = curr_precond->value;
        int precond_var_no = parent_vars[local_var];

        if (current_val == precond_value)
            continue;

        LocalProblem * &child_problem = problem_index[precond_var_no][current_val];
        if (!child_problem) {
            child_problem = new LocalProblem(precond_var_no);
            g_HACK->local_problems.push_back(child_problem);
        }

        if (!child_problem->is_initialized())
            child_problem->initialize(source->priority(), current_val, state);
        LocalProblemNode *cond_node = &child_problem->nodes[precond_value];
        if (cond_node->expanded) {
            target_cost += cond_node->cost;
            if (target->cost <= target_cost) {
                // Transition cannot find a shorter path to target.
                return;
            }
        } else {
            cond_node->add_to_waiting_list(this);
            ++unreached_conditions;
        }
    }
    try_to_fire();
}

void LocalTransition::on_condition_reached(int cost) {
    assert(unreached_conditions);
    --unreached_conditions;
    target_cost += cost;

    try_to_fire();
}

LocalProblemNode::LocalProblemNode(LocalProblem *owner_,
                                   int children_state_size) {
    owner = owner_;
    cost = -1;
    expanded = false;
    reached_by = 0;
    children_state.resize(children_state_size, -1);
}

void LocalProblemNode::add_to_waiting_list(LocalTransition *transition) {
    waiting_list.push_back(transition);
}

void LocalProblemNode::on_expand() {
    expanded = true;
    // Set children state unless this was an initial node.
    if (reached_by) {
        LocalProblemNode *parent = reached_by->source;
        children_state = parent->children_state;
        const vector<LocalAssignment> &precond = reached_by->label->precond;
        for (int i = 0; i < precond.size(); i++)
            children_state[precond[i].local_var] = precond[i].value;
        const vector<LocalAssignment> &effect = reached_by->label->effect;
        for (int i = 0; i < effect.size(); i++)
            children_state[effect[i].local_var] = effect[i].value;
        if (parent->reached_by)
            reached_by = parent->reached_by;
    }
    for (int i = 0; i < waiting_list.size(); i++)
        waiting_list[i]->on_condition_reached(cost);
    waiting_list.clear();
}

void LocalProblem::build_nodes_for_variable(int var_no) {
    DomainTransitionGraph *dtg = g_transition_graphs[var_no];

    causal_graph_parents = &dtg->cea_parents;

    int num_parents = causal_graph_parents->size();
    for (int value = 0; value < g_variable_domain[var_no]; value++)
        nodes.push_back(LocalProblemNode(this, num_parents));

    // Compile the DTG arcs into LocalTransition objects.
    for (int value = 0; value < nodes.size(); value++) {
        LocalProblemNode &node = nodes[value];
        const ValueNode &dtg_node = dtg->nodes[value];
        for (int i = 0; i < dtg_node.transitions.size(); i++) {
            const ValueTransition &dtg_trans = dtg_node.transitions[i];
            int target_value = dtg_trans.target->value;
            LocalProblemNode &target = nodes[target_value];
            for (int j = 0; j < dtg_trans.cea_labels.size(); j++) {
                const ValueTransitionLabel &label = dtg_trans.cea_labels[j];
                int action_cost = g_HACK->get_adjusted_cost(*label.op);
                LocalTransition trans(&node, &target, &label, action_cost);
                node.outgoing_transitions.push_back(trans);
            }
        }
    }
}

void LocalProblem::build_nodes_for_goal() {
    // TODO: We have a small memory leak here. Could be fixed by
    // making two LocalProblem classes with a virtual destructor.
    causal_graph_parents = new vector<int>;
    for (int i = 0; i < g_goal.size(); i++)
        causal_graph_parents->push_back(g_goal[i].first);

    for (int value = 0; value < 2; value++)
        nodes.push_back(LocalProblemNode(this, g_goal.size()));

    vector<LocalAssignment> goals;
    for (int goal_no = 0; goal_no < g_goal.size(); goal_no++) {
        int goal_value = g_goal[goal_no].second;
        goals.push_back(LocalAssignment(goal_no, goal_value));
    }
    vector<LocalAssignment> no_effects;
    ValueTransitionLabel *label = new ValueTransitionLabel(0, goals, no_effects);
    LocalTransition trans(&nodes[0], &nodes[1], label, 0);
    nodes[0].outgoing_transitions.push_back(trans);
}

LocalProblem::LocalProblem(int var_no) {
    base_priority = -1;
    if (var_no == -1)
        build_nodes_for_goal();
    else
        build_nodes_for_variable(var_no);
}

void LocalProblem::initialize(int base_priority_, int start_value,
                              const State &state) {
    assert(!is_initialized());
    base_priority = base_priority_;

    for (int to_value = 0; to_value < nodes.size(); to_value++) {
        nodes[to_value].expanded = false;
        nodes[to_value].cost = numeric_limits<int>::max();
        nodes[to_value].waiting_list.clear();
        nodes[to_value].reached_by = 0;
    }

    LocalProblemNode *start = &nodes[start_value];
    start->cost = 0;
    for (int i = 0; i < causal_graph_parents->size(); i++)
        start->children_state[i] = state[(*causal_graph_parents)[i]];

    g_HACK->add_to_heap(start);
}

void LocalProblemNode::mark_helpful_transitions(const State &state) {
    assert(cost >= 0 && cost < numeric_limits<int>::max());
    if (reached_by) {
        LocalTransition *first_on_path = reached_by;
        reached_by = 0; // Clear to avoid revisiting this node later.
        if (first_on_path->target_cost == first_on_path->action_cost) {
            // Transition possibly applicable.
            const Operator *op = first_on_path->label->op;
            if (g_min_action_cost != 0 || op->is_applicable(state)) {
                // If there are no zero-cost actions, the target_cost/
                // action_cost test above already guarantees applicability.
                assert(!op->is_axiom());
                g_HACK->set_preferred(op);
            }
        } else {
            // Recursively compute helpful transitions for precondition variables.
            const vector<LocalAssignment> &precond = first_on_path->label->precond;
            int *parent_vars = &*owner->causal_graph_parents->begin();
            for (int i = 0; i < precond.size(); i++) {
                int precond_value = precond[i].value;
                int local_var = precond[i].local_var;
                int precond_var_no = parent_vars[local_var];
                if (state[precond_var_no] == precond_value)
                    continue;
                LocalProblemNode *child_node = &g_HACK->get_local_problem(
                    precond_var_no, state[precond_var_no])->nodes[precond_value];
                child_node->mark_helpful_transitions(state);
            }
        }
    }
}

ContextEnhancedAdditiveHeuristic::ContextEnhancedAdditiveHeuristic(
    const HeuristicOptions &options) : Heuristic(options) {
    if (g_HACK)
        abort();
    g_HACK = this;
    goal_problem = 0;
    goal_node = 0;
}

ContextEnhancedAdditiveHeuristic::~ContextEnhancedAdditiveHeuristic() {
    delete goal_problem;
    for (int i = 0; i < local_problems.size(); i++)
        delete local_problems[i];
}

void ContextEnhancedAdditiveHeuristic::initialize() {
    assert(goal_problem == 0);
    cout << "Initializing context-enhanced additive heuristic..." << endl;

    int num_variables = g_variable_domain.size();

    goal_problem = new LocalProblem;
    goal_node = &goal_problem->nodes[1];

    local_problem_index.resize(num_variables);
    for (int var_no = 0; var_no < num_variables; var_no++) {
        int num_values = g_variable_domain[var_no];
        local_problem_index[var_no].resize(num_values, 0);
    }
}

int ContextEnhancedAdditiveHeuristic::compute_heuristic(const State &state) {
    initialize_heap();
    goal_problem->base_priority = -1;
    for (int i = 0; i < local_problems.size(); i++)
        local_problems[i]->base_priority = -1;

    goal_problem->initialize(0, 0, state);

    int heuristic = compute_costs(state);

    if (heuristic != DEAD_END && heuristic != 0)
        goal_node->mark_helpful_transitions(state);

    return heuristic;
}

void ContextEnhancedAdditiveHeuristic::initialize_heap() {
    node_queue.clear();
}

int ContextEnhancedAdditiveHeuristic::compute_costs(const State &state) {
    while (!node_queue.empty()) {
        pair<int, LocalProblemNode *> top_pair = node_queue.pop();
        int curr_priority = top_pair.first;
        LocalProblemNode *node = top_pair.second;

        assert(node->owner->is_initialized());
        if (node->priority() < curr_priority)
            continue;
        if (node == goal_node)
            return node->cost;

        assert(node->priority() == curr_priority);
        node->on_expand();
        for (int i = 0; i < node->outgoing_transitions.size(); i++)
            node->outgoing_transitions[i].on_source_expanded(state);
    }
    return DEAD_END;
}

ScalarEvaluator *ContextEnhancedAdditiveHeuristic::create(
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
        return new ContextEnhancedAdditiveHeuristic(common_options);
    }
}
