#include "cea_heuristic.h"

#include "domain_transition_graph.h"
#include "globals.h"
#include "global_operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "global_state.h"

#include <cassert>
#include <limits>
#include <vector>
using namespace std;


/* Implementation notes:

   The main data structures are:
   - LocalProblem: a single "copy" of a domain transition graph, which
     is used to compute the costs of achieving all facts (v=d') for a
     fixed variable v starting from a fixed value d. So we can have at
     most |dom(v)| many local problems for any variable v. These are
     created lazily as needed.
   - LocalProblemNode: a single vertex in the domain transition graph
     represented by a LocalProblem. Knows what the successors in the
     graph are and keeps tracks of costs and helpful transitions for
     the node.
   - LocalTransition: a transition between two local problem nodes.
     Keeps track of how many unachieved preconditions there still are,
     what the cost of enabling the transition are and things like that.

   The following two design decisions might be worth revisiting:
   - Each local problem keeps its own copy of the graph itself
     (what is connected to what via which labels), even though this
     is not necessary. The "static" graph info and the "dynamic" info
     could be split, potentially saving quite a bit of memory.
   - The graph is encoded with reference cycles: each transition knows
     what its source node is, even though this is in a sense redundant
     (the source must be the node which holds the transition), and
     every node knows what its local problem is, which is similarly
     redundant (the local problem must be the one that holds this node).
     If we got rid of this, the main queue of the algorithm would need
     (LocalProblem *, LocalProblemNode *) pairs rather than straight
     node pointers, and the waiting lists would need to contain
     (LocalProblemNode *, LocalTransition *) pairs rather than straight
     transitions. So it's not clear if this would really save much, which
     is why we do not currently do it.
 */

namespace cea_heuristic {
struct LocalTransition {
    LocalProblemNode *source;
    LocalProblemNode *target;
    const ValueTransitionLabel *label;
    int action_cost;

    int target_cost;
    int unreached_conditions;

    LocalTransition(
        LocalProblemNode *source_, LocalProblemNode *target_,
        const ValueTransitionLabel *label_, int action_cost_)
        : source(source_), target(target_),
          label(label_), action_cost(action_cost_),
          target_cost(-1), unreached_conditions(-1) {
        // target_cost and unreached_cost are initialized by
        // expand_transition.
    }

    ~LocalTransition() {
    }
};


struct LocalProblemNode {
    // Attributes fixed during initialization.
    LocalProblem *owner;
    vector<LocalTransition> outgoing_transitions;

    // Dynamic attributes (modified during heuristic computation).
    int cost;
    bool expanded;
    vector<short> context;

    LocalTransition *reached_by;
    /* Before a node is expanded, reached_by is the "current best"
       transition leading to this node. After a node is expanded, the
       reached_by value of the parent is copied (unless the parent is
       the initial node), so that reached_by is the *first* transition
       on the optimal path to this node. This is useful for preferred
       operators. (The two attributes used to be separate, but this
       was a bit wasteful.) */

    vector<LocalTransition *> waiting_list;

    LocalProblemNode(LocalProblem *owner_, int context_size)
        : owner(owner_),
          cost(-1),
          expanded(false),
          context(context_size, -1),
          reached_by(0) {
    }

    ~LocalProblemNode() {
    }
};

struct LocalProblem {
    int base_priority;
    vector<LocalProblemNode> nodes;
    vector<int> *context_variables;
public:
    LocalProblem()
        : base_priority(-1) {
    }

    ~LocalProblem() {
    }
};

LocalProblem *ContextEnhancedAdditiveHeuristic::get_local_problem(
    int var_no, int value) {
    LocalProblem * &table_entry = local_problem_index[var_no][value];
    if (!table_entry) {
        table_entry = build_problem_for_variable(var_no);
        local_problems.push_back(table_entry);
    }
    return table_entry;
}

LocalProblem *ContextEnhancedAdditiveHeuristic::build_problem_for_variable(
    int var_no) const {
    LocalProblem *problem = new LocalProblem;

    DomainTransitionGraph *dtg = g_transition_graphs[var_no];

    problem->context_variables = &dtg->cea_parents;

    int num_parents = problem->context_variables->size();
    size_t num_values = g_variable_domain[var_no];
    problem->nodes.reserve(num_values);
    for (size_t value = 0; value < num_values; ++value)
        problem->nodes.push_back(LocalProblemNode(problem, num_parents));

    // Compile the DTG arcs into LocalTransition objects.
    for (size_t value = 0; value < num_values; ++value) {
        LocalProblemNode &node = problem->nodes[value];
        const ValueNode &dtg_node = dtg->nodes[value];
        for (size_t i = 0; i < dtg_node.transitions.size(); ++i) {
            const ValueTransition &dtg_trans = dtg_node.transitions[i];
            int target_value = dtg_trans.target->value;
            LocalProblemNode &target = problem->nodes[target_value];
            for (size_t j = 0; j < dtg_trans.cea_labels.size(); ++j) {
                const ValueTransitionLabel &label = dtg_trans.cea_labels[j];
                int action_cost = get_adjusted_cost(*label.op);
                LocalTransition trans(&node, &target, &label, action_cost);
                node.outgoing_transitions.push_back(trans);
            }
        }
    }
    return problem;
}

LocalProblem *ContextEnhancedAdditiveHeuristic::build_problem_for_goal() const {
    LocalProblem *problem = new LocalProblem;

    problem->context_variables = new vector<int>;
    for (size_t i = 0; i < g_goal.size(); ++i)
        problem->context_variables->push_back(g_goal[i].first);

    for (size_t value = 0; value < 2; ++value)
        problem->nodes.push_back(LocalProblemNode(problem, g_goal.size()));

    vector<LocalAssignment> goals;
    for (size_t goal_no = 0; goal_no < g_goal.size(); ++goal_no) {
        int goal_value = g_goal[goal_no].second;
        goals.push_back(LocalAssignment(goal_no, goal_value));
    }
    vector<LocalAssignment> no_effects;
    ValueTransitionLabel *label = new ValueTransitionLabel(0, goals, no_effects);
    LocalTransition trans(&problem->nodes[0], &problem->nodes[1], label, 0);
    problem->nodes[0].outgoing_transitions.push_back(trans);
    return problem;
}

int ContextEnhancedAdditiveHeuristic::get_priority(
    LocalProblemNode *node) const {
    /* Nodes have both a "cost" and a "priority", which are related.
       The cost is an estimate of how expensive it is to reach this
       node. The "priority" is the lowest cost value in the overall
       cost computation for which this node will be important. It is
       essentially the sum of the cost and a local-problem-specific
       "base priority", which depends on where this local problem is
       needed for the overall computation. */
    return node->owner->base_priority + node->cost;
}

inline void ContextEnhancedAdditiveHeuristic::initialize_heap() {
    node_queue.clear();
}

inline void ContextEnhancedAdditiveHeuristic::add_to_heap(
    LocalProblemNode *node) {
    node_queue.push(get_priority(node), node);
}

bool ContextEnhancedAdditiveHeuristic::is_local_problem_set_up(
    const LocalProblem *problem) const {
    return problem->base_priority != -1;
}

void ContextEnhancedAdditiveHeuristic::set_up_local_problem(
    LocalProblem *problem, int base_priority,
    int start_value, const GlobalState &state) {
    assert(problem->base_priority == -1);
    problem->base_priority = base_priority;

    vector<LocalProblemNode> &nodes = problem->nodes;
    for (size_t to_value = 0; to_value < nodes.size(); ++to_value) {
        nodes[to_value].expanded = false;
        nodes[to_value].cost = numeric_limits<int>::max();
        nodes[to_value].waiting_list.clear();
        nodes[to_value].reached_by = 0;
    }

    LocalProblemNode *start = &nodes[start_value];
    start->cost = 0;
    for (size_t i = 0; i < problem->context_variables->size(); ++i)
        start->context[i] = state[(*problem->context_variables)[i]];

    add_to_heap(start);
}

void ContextEnhancedAdditiveHeuristic::try_to_fire_transition(
    LocalTransition *trans) {
    if (!trans->unreached_conditions) {
        LocalProblemNode *target = trans->target;
        if (trans->target_cost < target->cost) {
            target->cost = trans->target_cost;
            target->reached_by = trans;
            add_to_heap(target);
        }
    }
}

void ContextEnhancedAdditiveHeuristic::expand_node(LocalProblemNode *node) {
    node->expanded = true;
    // Set context unless this was an initial node.
    LocalTransition *reached_by = node->reached_by;
    if (reached_by) {
        LocalProblemNode *parent = reached_by->source;
        vector<short> &context = node->context;
        context = parent->context;
        const vector<LocalAssignment> &precond = reached_by->label->precond;
        for (size_t i = 0; i < precond.size(); ++i)
            context[precond[i].local_var] = precond[i].value;
        const vector<LocalAssignment> &effect = reached_by->label->effect;
        for (size_t i = 0; i < effect.size(); ++i)
            context[effect[i].local_var] = effect[i].value;
        if (parent->reached_by)
            node->reached_by = parent->reached_by;
    }
    for (size_t i = 0; i < node->waiting_list.size(); ++i) {
        LocalTransition *trans = node->waiting_list[i];
        assert(trans->unreached_conditions);
        --trans->unreached_conditions;
        trans->target_cost += node->cost;
        try_to_fire_transition(trans);
    }
    node->waiting_list.clear();
}

void ContextEnhancedAdditiveHeuristic::expand_transition(
    LocalTransition *trans, const GlobalState &state) {
    /* Called when the source of trans is reached by Dijkstra
       exploration. Try to compute cost for the target of the
       transition from the source cost, action cost, and set-up costs
       for the conditions on the label. The latter may yet be unknown,
       in which case we "subscribe" to the waiting list of the node
       that will tell us the correct value. */

    assert(trans->source->cost >= 0);
    assert(trans->source->cost < numeric_limits<int>::max());

    trans->target_cost = trans->source->cost + trans->action_cost;

    if (trans->target->cost <= trans->target_cost) {
        // Transition cannot find a shorter path to target.
        return;
    }

    trans->unreached_conditions = 0;
    const vector<LocalAssignment> &precond = trans->label->precond;

    vector<LocalAssignment>::const_iterator
        curr_precond = precond.begin(),
        last_precond = precond.end();

    short *context = &trans->source->context.front();
    int *parent_vars = &*trans->source->owner->context_variables->begin();

    for (; curr_precond != last_precond; ++curr_precond) {
        int local_var = curr_precond->local_var;
        int current_val = context[local_var];
        int precond_value = curr_precond->value;
        int precond_var_no = parent_vars[local_var];

        if (current_val == precond_value)
            continue;

        LocalProblem *subproblem = get_local_problem(
            precond_var_no, current_val);

        if (!is_local_problem_set_up(subproblem)) {
            set_up_local_problem(
                subproblem, get_priority(trans->source), current_val, state);
        }

        LocalProblemNode *cond_node = &subproblem->nodes[precond_value];
        if (cond_node->expanded) {
            trans->target_cost += cond_node->cost;
            if (trans->target->cost <= trans->target_cost) {
                // Transition cannot find a shorter path to target.
                return;
            }
        } else {
            cond_node->waiting_list.push_back(trans);
            ++trans->unreached_conditions;
        }
    }
    try_to_fire_transition(trans);
}

int ContextEnhancedAdditiveHeuristic::compute_costs(const GlobalState &state) {
    while (!node_queue.empty()) {
        pair<int, LocalProblemNode *> top_pair = node_queue.pop();
        int curr_priority = top_pair.first;
        LocalProblemNode *node = top_pair.second;

        assert(is_local_problem_set_up(node->owner));
        if (get_priority(node) < curr_priority)
            continue;
        if (node == goal_node)
            return node->cost;

        assert(get_priority(node) == curr_priority);
        expand_node(node);
        for (size_t i = 0; i < node->outgoing_transitions.size(); ++i)
            expand_transition(&node->outgoing_transitions[i], state);
    }
    return DEAD_END;
}

void ContextEnhancedAdditiveHeuristic::mark_helpful_transitions(
    LocalProblem *problem, LocalProblemNode *node, const GlobalState &state) {
    assert(node->cost >= 0 && node->cost < numeric_limits<int>::max());
    LocalTransition *first_on_path = node->reached_by;
    if (first_on_path) {
        node->reached_by = 0; // Clear to avoid revisiting this node later.
        if (first_on_path->target_cost == first_on_path->action_cost) {
            // Transition possibly applicable.
            const GlobalOperator *op = first_on_path->label->op;
            if (g_min_action_cost != 0 || op->is_applicable(state)) {
                // If there are no zero-cost actions, the target_cost/
                // action_cost test above already guarantees applicability.
                assert(!op->is_axiom());
                set_preferred(op);
            }
        } else {
            // Recursively compute helpful transitions for preconditions.
            const vector<LocalAssignment> &precond = first_on_path->label->precond;
            int *context_vars = &*problem->context_variables->begin();
            for (size_t i = 0; i < precond.size(); ++i) {
                int precond_value = precond[i].value;
                int local_var = precond[i].local_var;
                int precond_var_no = context_vars[local_var];
                if (state[precond_var_no] == precond_value)
                    continue;
                LocalProblem *subproblem = get_local_problem(
                    precond_var_no, state[precond_var_no]);
                LocalProblemNode *subnode = &subproblem->nodes[precond_value];
                mark_helpful_transitions(subproblem, subnode, state);
            }
        }
    }
}

void ContextEnhancedAdditiveHeuristic::initialize() {
    assert(goal_problem == 0);
    cout << "Initializing context-enhanced additive heuristic..." << endl;

    int num_variables = g_variable_domain.size();

    goal_problem = build_problem_for_goal();
    goal_node = &goal_problem->nodes[1];

    local_problem_index.resize(num_variables);
    for (size_t var_no = 0; var_no < num_variables; ++var_no) {
        int num_values = g_variable_domain[var_no];
        local_problem_index[var_no].resize(num_values, 0);
    }
}

int ContextEnhancedAdditiveHeuristic::compute_heuristic(const GlobalState &state) {
    initialize_heap();
    goal_problem->base_priority = -1;
    for (size_t i = 0; i < local_problems.size(); ++i)
        local_problems[i]->base_priority = -1;

    set_up_local_problem(goal_problem, 0, 0, state);

    int heuristic = compute_costs(state);

    if (heuristic != DEAD_END && heuristic != 0)
        mark_helpful_transitions(goal_problem, goal_node, state);

    return heuristic;
}

ContextEnhancedAdditiveHeuristic::ContextEnhancedAdditiveHeuristic(
    const Options &opts) : Heuristic(opts) {
    goal_problem = 0;
    goal_node = 0;
}

ContextEnhancedAdditiveHeuristic::~ContextEnhancedAdditiveHeuristic() {
    if (goal_problem) {
        delete goal_problem->context_variables;
        delete goal_problem->nodes[0].outgoing_transitions[0].label;
    }
    delete goal_problem;

    for (size_t i = 0; i < local_problems.size(); ++i)
        delete local_problems[i];
}

bool ContextEnhancedAdditiveHeuristic::dead_ends_are_reliable() const {
    return false;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Context-enhanced additive heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support(
        "axioms",
        "supported (in the sense that the planner won't complain -- "
        "handling of axioms might be very stupid "
        "and even render the heuristic unsafe)");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "no");
    parser.document_property("preferred operators", "yes");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return 0;
    else
        return new ContextEnhancedAdditiveHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cea", _parse);
}
