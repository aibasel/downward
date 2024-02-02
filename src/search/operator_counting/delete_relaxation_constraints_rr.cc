#include "delete_relaxation_constraints_rr.h"

#include "../algorithms/priority_queues.h"
#include "../lp/lp_solver.h"
#include "../plugins/plugin.h"
#include "../algorithms/priority_queues.h"
#include "../task_proxy.h"
#include "../utils/markup.h"

#include <cassert>
#include <optional>
#include <vector>

using namespace std;

namespace operator_counting {
class VEGraph {
    struct Node {
        vector<FactPair> predecessors;
        vector<FactPair> successors;
        bool is_eliminated = false;
        int in_degree;
    };

    /*
      Vertex Elimination Graphs have one node per fact. We index them by
      variable and value.
    */
    vector<vector<Node>> nodes;
    vector<tuple<FactPair, FactPair, FactPair>> delta;
    priority_queues::AdaptiveQueue<FactPair> elimination_queue;

    Node &get_node(FactPair fact) {
        return nodes[fact.var][fact.value];
    }

    const Node &get_node(FactPair fact) const {
        return nodes[fact.var][fact.value];
    }

    void add_edge(FactPair from_fact, FactPair to_fact) {
        get_node(from_fact).successors.push_back(to_fact);
        get_node(to_fact).predecessors.push_back(from_fact);
    }

    void push_fact(FactPair fact) {
        Node &node = get_node(fact);
        if (node.is_eliminated) {
            return;
        }
        int in_degree = 0;
        for (FactPair predecessor : node.predecessors) {
            if (!get_node(predecessor).is_eliminated) {
                ++in_degree;
            }
        }
        node.in_degree = in_degree;
        elimination_queue.push(in_degree, fact);
    }

    optional<FactPair> pop_fact() {
        while (!elimination_queue.empty()) {
            const auto [key, fact] = elimination_queue.pop();
            Node &node = get_node(fact);
            if (node.in_degree == key) {
                return fact;
            }
        }
        return nullopt;
    }

    void eliminate(FactPair fact) {
        Node &node = get_node(fact);
        /*
          When eliminating the given fact from the graph, we add shorcut edges
          from all its (non-eliminated) predecessors, to all its
          (non-eliminated) successors.
        */
        vector<tuple<FactPair, FactPair, FactPair>> new_shortcuts;
        for (FactPair predecessor : node.predecessors) {
            if (get_node(predecessor).is_eliminated) {
                continue;
            }
            for (FactPair successor : node.successors) {
                if (get_node(successor).is_eliminated) {
                    continue;
                }
                // TODO avoid linear scan
                const vector<FactPair> pre_succs = get_node(predecessor).successors;
                if (successor != predecessor && find(pre_succs.begin(), pre_succs.end(), successor) == pre_succs.end()) {
                    new_shortcuts.push_back(make_tuple(predecessor, fact, successor));
                }
            }
        }
        node.is_eliminated = true;

        for (tuple<FactPair, FactPair, FactPair> shortcut : new_shortcuts) {
            auto [from, _, to] = shortcut;
            add_edge(from, to);
            delta.push_back(shortcut);
        }

        /*
          The elimination can affect the priority queue which uses the number of
          incoming edges from non-eliminated nodes as a key. However, this can
          only change for successors of 'fact'. We add them back into the queue
          with updated keys and lazily filter out the outdated values.
        */
        for (FactPair successor : node.successors) {
            if (!get_node(successor).is_eliminated) {
                push_fact(successor);
            }
        }
    }

    void initialize_queue() {
        int num_vars = nodes.size();
        for (int var = 0; var < num_vars; ++var) {
            int num_values = nodes[var].size();
            for (int val = 0; val < num_values; ++val) {
                push_fact(FactPair(var, val));
            }
        }
    }

public:
    explicit VEGraph(const TaskProxy &task_proxy) {
        nodes.resize(task_proxy.get_variables().size());
        for (VariableProxy var : task_proxy.get_variables()) {
            nodes[var.get_id()].resize(var.get_domain_size());
        }
        for (OperatorProxy op : task_proxy.get_operators()) {
            for (FactProxy pre_proxy : op.get_preconditions()) {
                FactPair pre = pre_proxy.get_pair();
                for (EffectProxy eff_proxy : op.get_effects()) {
                    FactPair eff = eff_proxy.get_fact().get_pair();
                    if (pre != eff) {
                        add_edge(pre, eff);
                    }
                }
            }
        }
    }

    void run() {
        initialize_queue();
        while (optional<FactPair> fact = pop_fact()) {
            eliminate(*fact);
        }
    }

    const vector<tuple<FactPair, FactPair, FactPair>> &get_delta() const {
        return delta;
    }

    vector<pair<FactPair, FactPair>> copy_edges() const {
        vector<pair<FactPair, FactPair>> edges;
        int num_vars = nodes.size();
        for (int var = 0; var < num_vars; ++var) {
            int num_values = nodes[var].size();
            for (int val = 0; val < num_values; ++val) {
                FactPair fact(var, val);
                for (FactPair succ : get_node(fact).successors) {
                    edges.emplace_back(fact, succ);
                }
            }
        }
        return edges;
    }
};

static void add_lp_variables(int count, LPVariables &variables,
                             vector<int> &indices, double lower, double upper,
                             double objective, bool is_integer) {
    for (int i = 0; i < count; ++i) {
        indices.push_back(variables.size());
        variables.emplace_back(lower, upper, objective, is_integer);
    }
}

DeleteRelaxationConstraintsRR::DeleteRelaxationConstraintsRR(
    const plugins::Options &opts)
    : use_time_vars(opts.get<bool>("use_time_vars")),
      use_integer_vars(opts.get<bool>("use_integer_vars")) {}

int DeleteRelaxationConstraintsRR::get_var_f_defined(FactPair f) {
    return lp_var_id_f_defined[f.var][f.value];
}

int DeleteRelaxationConstraintsRR::get_var_f_maps_to(FactPair f,
                                                     const OperatorProxy &op) {
    return lp_var_id_f_maps_to.at(make_tuple(f.var, f.value, op.get_id()));
}

int DeleteRelaxationConstraintsRR::get_constraint_id(FactPair f) {
    return lp_con_id_f_defined[f.var][f.value];
}

bool DeleteRelaxationConstraintsRR::is_in_effect(FactPair f,
                                                 const OperatorProxy &op) {
    for (EffectProxy eff : op.get_effects()) {
        if (eff.get_fact().get_pair() == f) {
            return true;
        }
    }
    return false;
}

bool DeleteRelaxationConstraintsRR::is_in_precondition(
    FactPair f, const OperatorProxy &op) {
    for (FactProxy pre : op.get_preconditions()) {
        if (pre.get_pair() == f) {
            return true;
        }
    }
    return false;
}

void DeleteRelaxationConstraintsRR::create_auxiliary_variables(
    const TaskProxy &task_proxy, LPVariables &variables, const VEGraph &ve_graph) {
    OperatorsProxy ops = task_proxy.get_operators();
    VariablesProxy vars = task_proxy.get_variables();
    int num_vars = vars.size();

    lp_var_id_f_defined.resize(num_vars);
    for (VariableProxy var : vars) {
        int var_id = var.get_id();
        // Add f_p variable.
        add_lp_variables(var.get_domain_size(), variables,
                         lp_var_id_f_defined[var_id], 0, 1, 0,
                         use_integer_vars);
        // Add f_{p,a} variables.
        for (int value = 0; value < var.get_domain_size(); ++value) {
            for (OperatorProxy op : ops) {
                if (is_in_effect(FactPair(var_id, value), op)) {
                    lp_var_id_f_maps_to.emplace(
                        make_pair(make_tuple(var_id, value, op.get_id()),
                                  variables.size()));
                    variables.emplace_back(0, 1, 0, use_integer_vars);
                }
            }
        }
    }

    for (pair<FactPair, FactPair> edge : ve_graph.copy_edges()) {
        lp_var_id_edge.emplace(make_pair(edge, variables.size()));
        variables.emplace_back(0, 1, 0, use_integer_vars);
    }
}

void DeleteRelaxationConstraintsRR::create_constraints(
    const TaskProxy &task_proxy, lp::LinearProgram &lp, const VEGraph &ve_graph) {
    LPVariables &variables = lp.get_variables();
    LPConstraints &constraints = lp.get_constraints();
    double infinity = lp.get_infinity();
    OperatorsProxy ops = task_proxy.get_operators();
    VariablesProxy vars = task_proxy.get_variables();

    // Constraint (2) in paper.
    lp_con_id_f_defined.resize(vars.size());
    for (VariableProxy var_p : vars) {
        lp_con_id_f_defined[var_p.get_id()].resize(var_p.get_domain_size());
        for (int value_p = 0; value_p < var_p.get_domain_size(); ++value_p) {
            lp_con_id_f_defined[var_p.get_id()][value_p] = constraints.size();
            constraints.emplace_back(0, 0);
            FactPair fact_p(var_p.get_id(), value_p);
            constraints.back().insert(get_var_f_defined(fact_p), 1);
            for (OperatorProxy op : ops) {
                if (is_in_effect(fact_p, op))
                    constraints.back().insert(get_var_f_maps_to(fact_p, op),
                                              -1);
            }
        }
    }

    // Constraint (3) in paper.
    for (VariableProxy var_p : vars) {
        for (int value_p = 0; value_p < var_p.get_domain_size(); ++value_p) {
            FactPair fact_p(var_p.get_id(), value_p);
            for (VariableProxy var_q : vars) {
                for (int value_q = 0; value_q < var_q.get_domain_size();
                     ++value_q) {
                    FactPair fact_q(var_q.get_id(), value_q);
                    if (fact_q != fact_p)
                        break;
                    constraints.emplace_back(0, 1);
                    constraints.back().insert(get_var_f_defined(fact_q), 1);
                    for (OperatorProxy op : ops) {
                        if (is_in_precondition(fact_q, op) &&
                            is_in_effect(fact_p, op)) {
                            constraints.back().insert(
                                get_var_f_maps_to(fact_p, op), -1);
                        }
                    }
                }
            }
        }
    }

    // Constraint (4) in paper.
    for (FactProxy goal : task_proxy.get_goals()) {
        variables[get_var_f_defined(goal.get_pair())].lower_bound = 1;
    }

    // Constraint (5) in paper.
    for (OperatorProxy op : ops) {
        for (EffectProxy eff : op.get_effects()) {
            FactPair fact_p = eff.get_fact().get_pair();
            constraints.emplace_back(0, infinity);
            constraints.back().insert(get_var_f_maps_to(fact_p, op), -1);
            constraints.back().insert(op.get_id(), 1);
        }
    }

    // Constraint (6) in paper.
    for (OperatorProxy op : ops) {
        for (FactProxy pre_proxy : op.get_preconditions()) {
            FactPair pre = pre_proxy.get_pair();
            for (EffectProxy eff_proxy : op.get_effects()) {
                FactPair eff = eff_proxy.get_fact().get_pair();
                constraints.emplace_back(0, infinity);
                constraints.back().insert(
                    lp_var_id_edge.at(make_pair(pre, eff)), 1);
                constraints.back().insert(get_var_f_maps_to(eff, op), -1);
            }
        }
    }

    // Constraint (7) in paper.
    /*
      TODO: Consider storing the result of copy_edges in VEGraph instead of
      computing it twice (here and in create_auxiliary_variables)
    */
    for (pair<FactPair, FactPair> &edge : ve_graph.copy_edges()) {
        auto reverse_edge_it =
            lp_var_id_edge.find(make_pair(edge.second, edge.first));
        if (reverse_edge_it == lp_var_id_edge.end())
            continue;
        int edge_id = lp_var_id_edge.at(edge);
        int reverse_edge_id = reverse_edge_it->second;
        constraints.emplace_back(-1, infinity);
        constraints.back().insert(edge_id, -1);
        constraints.back().insert(reverse_edge_id, -1);
    }

    // Constraint (8) in paper.
    for (const tuple<FactPair, FactPair, FactPair> &edge_triple :
         ve_graph.get_delta()) {
        constraints.emplace_back(-1, infinity);
        constraints.back().insert(
            lp_var_id_edge.at(
                make_pair(get<0>(edge_triple), get<1>(edge_triple))),
            -1);
        constraints.back().insert(
            lp_var_id_edge.at(
                make_pair(get<1>(edge_triple), get<2>(edge_triple))),
            -1);
        constraints.back().insert(
            lp_var_id_edge.at(
                make_pair(get<0>(edge_triple), get<2>(edge_triple))),
            1);
    }

    /*
      TODO: Implement constraint (9).
      - define ternary option to replace use_time_vars and use_integer_vars
      - create timing variables
      - create constraint
    */
    /*
      TODO: Make sure that objects that are only needed for constraint
      generation only exist in that context and not beyond. In particular there
      are lp_var_id_maps that are currently member variables, but should not be.
    */
}

void DeleteRelaxationConstraintsRR::initialize_constraints(
    const shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) {
    TaskProxy task_proxy(*task);
    VEGraph ve_graph(task_proxy);
    ve_graph.run();
    create_auxiliary_variables(task_proxy, lp.get_variables(), ve_graph);
    create_constraints(task_proxy, lp, ve_graph);
}

bool DeleteRelaxationConstraintsRR::update_constraints(
    const State &state, lp::LPSolver &lp_solver) {
    // Unset old bounds.
    int con_id;
    for (FactPair f : last_state) {
        con_id = get_constraint_id(f);
        lp_solver.set_constraint_lower_bound(con_id, 0);
        lp_solver.set_constraint_upper_bound(con_id, 0);
    }
    last_state.clear();
    // Set new bounds.
    for (FactProxy f : state) {
        con_id = get_constraint_id(f.get_pair());
        lp_solver.set_constraint_lower_bound(con_id, 1);
        lp_solver.set_constraint_upper_bound(con_id, 1);
        last_state.push_back(f.get_pair());
    }
    return false;
}

class DeleteRelaxationConstraintsRRFeature
    : public plugins::TypedFeature<ConstraintGenerator,
                                   DeleteRelaxationConstraintsRR> {
public:
    DeleteRelaxationConstraintsRRFeature()
        : TypedFeature("delete_relaxation_constraints_rr") {
        document_title(
            "Delete relaxation constraints from Rankooh and Rintanen");
        document_synopsis(
            "Operator-counting constraints based on the delete relaxation. By "
            "default the constraints encode an easy-to-compute relaxation of "
            "h^+^. "
            "With the right settings, these constraints can be used to compute "
            "the "
            "optimal delete-relaxation heuristic h^+^ (see example below). "
            "For details, see" +
            utils::format_journal_reference(
                {"Masood Feyzbakhsh Rankooh", "Jussi Rintanen"},
                "Efficient Computation and Informative Estimation of"
                "h+ by Integer and Linear Programming"
                "",
                "https://ojs.aaai.org/index.php/ICAPS/article/view/19787/19546",
                "Proceedings of the Thirty-Second International Conference on "
                "Automated Planning and Scheduling (ICAPS2022)",
                "32", "71-79", "2022"));

        add_option<bool>(
            "use_time_vars",
            "use variables for time steps. With these additional variables the "
            "constraints enforce an order between the selected operators.",
            "false");
        add_option<bool>(
            "use_integer_vars",
            "restrict auxiliary variables to integer values. These variables "
            "encode whether operators are used, facts are reached, which "
            "operator "
            "first achieves which fact, and in which order the operators are "
            "used. "
            "Restricting them to integers generally improves the heuristic "
            "value "
            "at the cost of increased runtime.",
            "false");

        document_note(
            "Example",
            "To compute the optimal delete-relaxation heuristic h^+^, use\n"
            "{{{\noperatorcounting([delete_relaxation_constraints_rr(use_time_"
            "vars=true, "
            "use_integer_vars=true)], "
            "use_integer_operator_counts=true))\n}}}\n");
    }
};

static plugins::FeaturePlugin<DeleteRelaxationConstraintsRRFeature> _plugin;
}
