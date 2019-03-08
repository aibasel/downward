#ifndef HEURISTICS_DOMAIN_TRANSITION_GRAPH_H
#define HEURISTICS_DOMAIN_TRANSITION_GRAPH_H

#include "../task_proxy.h"

#include <cassert>
#include <unordered_map>
#include <vector>


namespace cea_heuristic {
class ContextEnhancedAdditiveHeuristic;
}

namespace cg_heuristic {
class CGHeuristic;
}

namespace domain_transition_graph {
struct LocalAssignment;
struct ValueNode;
struct ValueTransition;
struct ValueTransitionLabel;
class DomainTransitionGraph;

// Note: We do not use references but pointers to refer to the "parents" of
// transitions and nodes. This is because these structures could not be
// put into vectors otherwise.

class DTGFactory {
    using DTGs = std::vector<std::unique_ptr<DomainTransitionGraph>>;
    const TaskProxy &task_proxy;
    bool collect_transition_side_effects;
    std::function<bool(int, int)> pruning_condition;

    std::vector<utils::HashMap<std::pair<int, int>, int>> transition_index;
    std::vector<std::unordered_map<int, int>> global_to_local_var;

    void allocate_graphs_and_nodes(DTGs &dtgs);
    void initialize_index_structures(int num_dtgs);
    void create_transitions(DTGs &dtgs);
    void process_effect(const EffectProxy &eff, const OperatorProxy &op,
                        DTGs &dtgs);
    void update_transition_condition(const FactProxy &fact,
                                     DomainTransitionGraph *dtg,
                                     std::vector<LocalAssignment> &condition);
    void extend_global_to_local_mapping_if_necessary(
        DomainTransitionGraph *dtg, int global_var);
    void revert_new_local_vars(DomainTransitionGraph *dtg,
                               unsigned int first_local_var);
    ValueTransition *get_transition(int origin, int target,
                                    DomainTransitionGraph *dtg);
    void simplify_transitions(DTGs &dtgs);
    void simplify_labels(std::vector<ValueTransitionLabel> &labels);
    void collect_all_side_effects(DTGs &dtgs);
    void collect_side_effects(DomainTransitionGraph *dtg,
                              std::vector<ValueTransitionLabel> &labels);
    OperatorProxy get_op_for_label(const ValueTransitionLabel &label);

public:
    DTGFactory(const TaskProxy &task_proxy,
               bool collect_transition_side_effects,
               const std::function<bool(int, int)> &pruning_condition);

    DTGs build_dtgs();
};

struct LocalAssignment {
    short local_var;
    short value;

    LocalAssignment(int var, int val)
        : local_var(var), value(val) {
        // Check overflow.
        assert(local_var == var);
        assert(value == val);
    }
};

struct ValueTransitionLabel {
    int op_id;
    bool is_axiom;
    std::vector<LocalAssignment> precond;
    std::vector<LocalAssignment> effect;

    ValueTransitionLabel(int op_id, bool axiom,
                         const std::vector<LocalAssignment> &precond,
                         const std::vector<LocalAssignment> &effect)
        : op_id(op_id), is_axiom(axiom), precond(precond), effect(effect) {}
};

struct ValueTransition {
    ValueNode *target;
    std::vector<ValueTransitionLabel> labels;

    ValueTransition(ValueNode *targ)
        : target(targ) {}

    void simplify(const TaskProxy &task_proxy);
};

struct ValueNode {
    DomainTransitionGraph *parent_graph;
    int value;
    std::vector<ValueTransition> transitions;

    std::vector<int> distances;
    std::vector<ValueTransitionLabel *> helpful_transitions;
    std::vector<int> children_state;
    ValueNode *reached_from;
    ValueTransitionLabel *reached_by;

    ValueNode(DomainTransitionGraph *parent, int val)
        : parent_graph(parent), value(val), reached_from(0), reached_by(0) {}
};

class DomainTransitionGraph {
    friend class cg_heuristic::CGHeuristic;
    friend class cea_heuristic::ContextEnhancedAdditiveHeuristic;
    friend class DTGFactory;

    int var;
    std::vector<ValueNode> nodes;

    int last_helpful_transition_extraction_time;

    std::vector<int> local_to_global_child;
    // used for mapping variables in conditions to their global index
    // (only needed for initializing child_state for the start node?)

    DomainTransitionGraph(const DomainTransitionGraph &other); // copying forbidden
public:
    DomainTransitionGraph(int var_index, int node_count);
};
}

#endif
