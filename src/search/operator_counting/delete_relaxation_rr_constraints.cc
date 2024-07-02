#include "delete_relaxation_rr_constraints.h"

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
    utils::HashSet<tuple<FactPair, FactPair, FactPair>> delta;
    utils::HashSet<pair<FactPair, FactPair>> edges;
    priority_queues::AdaptiveQueue<FactPair> elimination_queue;

    Node &get_node(FactPair fact) {
        return nodes[fact.var][fact.value];
    }

    const Node &get_node(FactPair fact) const {
        return nodes[fact.var][fact.value];
    }

    void add_edge(FactPair from_fact, FactPair to_fact) {
        pair<FactPair, FactPair> edge = make_pair(from_fact, to_fact);
        if (!edges.count(edge)) {
            get_node(from_fact).successors.push_back(to_fact);
            get_node(to_fact).predecessors.push_back(from_fact);
            edges.insert(edge);
        }
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
            if (node.in_degree == key && !node.is_eliminated) {
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
                if (predecessor != successor) {
                    new_shortcuts.push_back(make_tuple(predecessor, fact, successor));
                }
            }
        }
        node.is_eliminated = true;

        for (tuple<FactPair, FactPair, FactPair> shortcut : new_shortcuts) {
            auto [from, _, to] = shortcut;
            add_edge(from, to);
            delta.insert(shortcut);
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

    void construct_task_graph(const TaskProxy &task_proxy) {
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

    void initialize_queue(const TaskProxy &task_proxy) {
        for (VariableProxy var : task_proxy.get_variables()) {
            int num_values = var.get_domain_size();
            for (int val = 0; val < num_values; ++val) {
                push_fact(var.get_fact(val).get_pair());
            }
        }
    }

public:
    VEGraph(const TaskProxy &task_proxy) {
        construct_task_graph(task_proxy);
        initialize_queue(task_proxy);
        while (optional<FactPair> fact = pop_fact()) {
            eliminate(*fact);
        }
    }

    const utils::HashSet<tuple<FactPair, FactPair, FactPair>> &get_delta() const {
        return delta;
    }

    const utils::HashSet<pair<FactPair, FactPair>> &get_edges() const {
        return edges;
    }
};

int DeleteRelaxationRRConstraints::LPVariableIDs::id_of_fp(FactPair f) const {
    return fp_offsets[f.var] + f.value;
}

int DeleteRelaxationRRConstraints::LPVariableIDs::id_of_fpa(
    FactPair f, const OperatorProxy &op) const {
    return fpa_ids[op.get_id()].at(f);
}

int DeleteRelaxationRRConstraints::LPVariableIDs::id_of_e(
    pair<FactPair, FactPair> edge) const {
    return e_ids.at(edge);
}

int DeleteRelaxationRRConstraints::LPVariableIDs::has_e(
    pair<FactPair, FactPair> edge) const {
    return e_ids.find(edge) != e_ids.end();
}

int DeleteRelaxationRRConstraints::LPVariableIDs::id_of_t(FactPair f) const {
    return t_offsets[f.var] + f.value;
}

DeleteRelaxationRRConstraints::DeleteRelaxationRRConstraints(
    const plugins::Options &opts)
    : acyclicity_type(opts.get<AcyclicityType>("acyclicity_type")),
      use_integer_vars(opts.get<bool>("use_integer_vars")) {
}

int DeleteRelaxationRRConstraints::get_constraint_id(FactPair f) const {
    return constraint_offsets[f.var] + f.value;
}

DeleteRelaxationRRConstraints::LPVariableIDs
DeleteRelaxationRRConstraints::create_auxiliary_variables(
    const TaskProxy &task_proxy, LPVariables &variables) const {
    OperatorsProxy ops = task_proxy.get_operators();
    VariablesProxy task_variables = task_proxy.get_variables();
    int num_vars = task_variables.size();
    LPVariableIDs lp_var_ids;

    // Add f_p variables.
    lp_var_ids.fp_offsets.reserve(num_vars);
    for (VariableProxy var : task_variables) {
        lp_var_ids.fp_offsets.push_back(variables.size());
        int num_values = var.get_domain_size();
        for (int value = 0; value < num_values; ++value) {
            variables.emplace_back(0, 1, 0, use_integer_vars);
#ifndef NDEBUG
            variables.set_name(variables.size() - 1,
                               "f_" + var.get_name() + "_"
                               + var.get_fact(value).get_name());
#endif
        }
    }

    // Add f_{p,a} variables.
    lp_var_ids.fpa_ids.resize(ops.size());
    for (OperatorProxy op : ops) {
        for (EffectProxy eff_proxy : op.get_effects()) {
            FactPair eff = eff_proxy.get_fact().get_pair();
            lp_var_ids.fpa_ids[op.get_id()][eff] = variables.size();
            variables.emplace_back(0, 1, 0, use_integer_vars);
#ifndef NDEBUG
            variables.set_name(variables.size() - 1,
                               "f_" + eff_proxy.get_fact().get_name()
                               + "_achieved_by_" + op.get_name());
#endif
        }
    }
    return lp_var_ids;
}

void DeleteRelaxationRRConstraints::create_auxiliary_variables_ve(
    const TaskProxy &task_proxy, const VEGraph &ve_graph, LPVariables &variables,
    DeleteRelaxationRRConstraints::LPVariableIDs &lp_var_ids) const {
    utils::unused_variable(task_proxy);
    // Add e_{i,j} variables.
    for (pair<FactPair, FactPair> edge : ve_graph.get_edges()) {
        lp_var_ids.e_ids[edge] = variables.size();
        variables.emplace_back(0, 1, 0, use_integer_vars);
#ifndef NDEBUG
        auto [f1, f2] = edge;
        FactProxy f1_proxy = task_proxy.get_variables()[f1.var].get_fact(f1.value);
        FactProxy f2_proxy = task_proxy.get_variables()[f2.var].get_fact(f2.value);
        variables.set_name(variables.size() - 1,
                           "e_" + f1_proxy.get_name()
                           + "_before_" + f2_proxy.get_name());
#endif
    }
}

void DeleteRelaxationRRConstraints::create_auxiliary_variables_tl(
    const TaskProxy &task_proxy, LPVariables &variables,
    DeleteRelaxationRRConstraints::LPVariableIDs &lp_var_ids) const {
    int num_facts = 0;
    for (VariableProxy var : task_proxy.get_variables()) {
        num_facts += var.get_domain_size();
    }

    lp_var_ids.t_offsets.resize(task_proxy.get_variables().size());
    for (VariableProxy var : task_proxy.get_variables()) {
        lp_var_ids.t_offsets[var.get_id()] = variables.size();
        int num_values = var.get_domain_size();
        for (int value = 0; value < num_values; ++value) {
            variables.emplace_back(1, num_facts, 0, use_integer_vars);
#ifndef NDEBUG
            variables.set_name(variables.size() - 1,
                               "t_" + var.get_fact(value).get_name());
#endif
        }
    }
}

void DeleteRelaxationRRConstraints::create_constraints(
    const TaskProxy &task_proxy,
    const DeleteRelaxationRRConstraints::LPVariableIDs &lp_var_ids,
    lp::LinearProgram &lp) {
    LPVariables &variables = lp.get_variables();
    LPConstraints &constraints = lp.get_constraints();
    double infinity = lp.get_infinity();
    OperatorsProxy ops = task_proxy.get_operators();
    VariablesProxy vars = task_proxy.get_variables();

    /*
      Constraint (2) in paper:

      f_p = [p in s] + sum_{a in A where p in add(a)} f_{p,a}
      for all facts p.

      Intuition: p is reached iff we selected exactly one achiever for it, or
      if it is true in state s.
      Implementation notes: we will set the state-dependent part ([p in s]) in
      the update function and leave the right-hand side at 0 for now. The first
      loop creates all constraints and adds the term "f_p", the second loop adds
      the terms f_{p,a} to the appropriate constraints.
    */
    constraint_offsets.reserve(vars.size());
    for (VariableProxy var_p : vars) {
        int var_id_p = var_p.get_id();
        constraint_offsets.push_back(constraints.size());
        for (int value_p = 0; value_p < var_p.get_domain_size(); ++value_p) {
            FactPair fact_p(var_id_p, value_p);
            lp::LPConstraint constraint(0, 0);
            constraint.insert(lp_var_ids.id_of_fp(fact_p), 1);
            constraints.push_back(move(constraint));
        }
    }
    for (OperatorProxy op : ops) {
        for (EffectProxy eff_proxy : op.get_effects()) {
            FactPair eff = eff_proxy.get_fact().get_pair();
            lp::LPConstraint &constraint = constraints[get_constraint_id(eff)];
            constraint.insert(lp_var_ids.id_of_fpa(eff, op), -1);
        }
    }

    /*
      Constraint (3) in paper:

      sum_{a in A where q in pre(a) and p in add(a)} f_{p,a} <= f_q
      for all facts p, q.

      Intuition: If q is the precondition of an action that is selected as an
      achiever for p, then q must be reached. (Also, at most one action may be
      selected as the achiever of p.)
      Implementation notes: if there is no action in the sum for a pair (p, q),
      the constraint trivializes to 0 <= f_q which is guaranteed by the variable
      bounds. We thus only loop over pairs (p, q) that occur as effect and
      precondition in some action.
    */
    utils::HashMap<pair<FactPair, FactPair>, int> constraint3_ids;
    for (OperatorProxy op : ops) {
        for (EffectProxy eff_proxy : op.get_effects()) {
            FactPair eff = eff_proxy.get_fact().get_pair();
            for (FactProxy pre_proxy : op.get_preconditions()) {
                FactPair pre = pre_proxy.get_pair();
                if (pre == eff) {
                    continue;
                }
                pair<FactPair, FactPair> key = make_pair(pre, eff);
                if (!constraint3_ids.contains(key)) {
                    constraint3_ids[key] = constraints.size();
                    lp::LPConstraint constraint(0, 1);
                    constraint.insert(lp_var_ids.id_of_fp(pre), 1);
                    constraints.push_back(move(constraint));
                }
                int constraint_id = constraint3_ids[key];
                lp::LPConstraint &constraint = constraints[constraint_id];
                constraint.insert(lp_var_ids.id_of_fpa(eff, op), -1);
            }
        }
    }

    /*
      Constraint (4) in paper:

      f_p = 1 for all goal facts p.

      Intuition: We have to reach all goal facts.
      Implementation notes: we don't add a constraint but instead raise the
      lower bound of the (binary) variable to 1. A further optimization step
      would be to replace all occurrences of f_p with 1 in all other constraints
      but this would be more complicated.
    */
    for (FactProxy goal : task_proxy.get_goals()) {
        variables[lp_var_ids.id_of_fp(goal.get_pair())].lower_bound = 1;
    }

    /*
      Constraint (5) in paper:

      f_{p,a} <= count_a for all a in A and p in add(a).

      Intuition: if we use an action as an achiever for some fact, we have to
      use it at least once.
      Implementation notes: the paper uses a binary variable f_a instead of the
      operator-counting variable count_a. We can make this change without
      problems as f_a does not occur in any other constraint.
    */
    for (OperatorProxy op : ops) {
        for (EffectProxy eff_proxy : op.get_effects()) {
            FactPair eff = eff_proxy.get_fact().get_pair();
            lp::LPConstraint constraint(0, infinity);
            constraint.insert(lp_var_ids.id_of_fpa(eff, op), -1);
            constraint.insert(op.get_id(), 1);
            constraints.push_back(move(constraint));
        }
    }
}

void DeleteRelaxationRRConstraints::create_constraints_ve(
    const TaskProxy &task_proxy, const VEGraph &ve_graph,
    const DeleteRelaxationRRConstraints::LPVariableIDs &lp_var_ids,
    lp::LinearProgram &lp) {
    LPConstraints &constraints = lp.get_constraints();
    double infinity = lp.get_infinity();
    OperatorsProxy ops = task_proxy.get_operators();

    /*
      Constraint (6) in paper:

      f_{p_j,a} <= e_{i,j} for all a in A, p_i in pre(a), and p_j in add(a).

      Intuition: if we use a as the achiever of p_j, then its preconditions (in
      particular p_i) must be achieved earlier than p_j.
    */
    for (OperatorProxy op : ops) {
        for (FactProxy pre_proxy : op.get_preconditions()) {
            FactPair pre = pre_proxy.get_pair();
            for (EffectProxy eff_proxy : op.get_effects()) {
                FactPair eff = eff_proxy.get_fact().get_pair();
                lp::LPConstraint constraint(0, infinity);
                constraint.insert(lp_var_ids.id_of_e(make_pair(pre, eff)), 1);
                constraint.insert(lp_var_ids.id_of_fpa(eff, op), -1);
                constraints.push_back(move(constraint));
            }
        }
    }

    /*
      Constraint (7) in paper:

      e_{i,j} + e_{j,i} <= 1 for all (p_i, p_j) in E_Pi^*.

      Intuition: if there is a 2-cycle in the elimination graph, we have to
      avoid it by either ordering i before j or vice versa.
      Implementation note: the paper is not explicit about this but the
      constraint only makes sense if the reverse edge is in the graph.
    */
    for (const pair<FactPair, FactPair> &edge : ve_graph.get_edges()) {
        pair<FactPair, FactPair> reverse_edge = make_pair(edge.second, edge.first);
        if (lp_var_ids.has_e(reverse_edge)) {
            lp::LPConstraint constraint(-infinity, 1);
            constraint.insert(lp_var_ids.id_of_e(edge), 1);
            constraint.insert(lp_var_ids.id_of_e(reverse_edge), 1);
            constraints.push_back(move(constraint));
        }
    }

    /*
      Constraint (8) in paper:

      e_{i,j} + e_{j,k} - 1 <= e_{i,k} for all (p_i, p_j, p_k) in Delta.

      Intuition: if we introduced shortcut edge (p_i, p_k) while eliminating p_j
      cycles involving the new edge represents cycles containing the edges
      (p_i, p_j) and (p_j, p_k). If we don't order p_i before p_k, we also may
      not have both p_i ordered before p_j, and p_j ordered before p_k.
    */
    for (auto [pi, pj, pk] : ve_graph.get_delta()) {
        lp::LPConstraint constraint(-infinity, 1);
        constraint.insert(lp_var_ids.id_of_e(make_pair(pi, pj)), 1);
        constraint.insert(lp_var_ids.id_of_e(make_pair(pj, pk)), 1);
        constraint.insert(lp_var_ids.id_of_e(make_pair(pi, pk)), -1);
        constraints.push_back(move(constraint));
    }
}

void DeleteRelaxationRRConstraints::create_constraints_tl(
    const TaskProxy &task_proxy,
    const DeleteRelaxationRRConstraints::LPVariableIDs &lp_var_ids,
    lp::LinearProgram &lp) {
    /*
      Constraint (9) in paper:

      t_i - t_j + 1 <= |P|(1 - f_{p_j, a})
      for all a in A, p_i in pre(a) and p_j in add(a)
      Equivalent form:
      t_i - t_j + |P|f_{p_j, a} <= |P| - 1

      Intuition: if a is used to achieve p_j and p_i is one of a's
      preconditions, we have to achieve p_i before p_j.
    */
    LPConstraints &constraints = lp.get_constraints();
    double infinity = lp.get_infinity();
    int num_facts = 0;
    for (VariableProxy var : task_proxy.get_variables()) {
        num_facts += var.get_domain_size();
    }

    for (OperatorProxy op : task_proxy.get_operators()) {
        for (FactProxy pre_proxy : op.get_preconditions()) {
            FactPair pre = pre_proxy.get_pair();
            for (EffectProxy eff_proxy : op.get_effects()) {
                FactPair eff = eff_proxy.get_fact().get_pair();
                if (pre == eff) {
                    // Prevail conditions are compiled away in the paper.
                    continue;
                }
                lp::LPConstraint constraint(-infinity, num_facts - 1);
                constraint.insert(lp_var_ids.id_of_t(pre), 1);
                constraint.insert(lp_var_ids.id_of_t(eff), -1);
                constraint.insert(lp_var_ids.id_of_fpa(eff, op), num_facts);
                constraints.push_back(move(constraint));
            }
        }
    }
}

void DeleteRelaxationRRConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) {
    TaskProxy task_proxy(*task);
    LPVariableIDs lp_var_ids = create_auxiliary_variables(
        task_proxy, lp.get_variables());
    create_constraints(task_proxy, lp_var_ids, lp);

    switch (acyclicity_type) {
    case AcyclicityType::VERTEX_ELIMINATION:
    {
        VEGraph ve_graph(task_proxy);
        create_auxiliary_variables_ve(
            task_proxy, ve_graph, lp.get_variables(), lp_var_ids);
        create_constraints_ve(task_proxy, ve_graph, lp_var_ids, lp);
        break;
    }
    case AcyclicityType::TIME_LABELS:
    {
        create_auxiliary_variables_tl(
            task_proxy, lp.get_variables(), lp_var_ids);
        create_constraints_tl(task_proxy, lp_var_ids, lp);
        break;
    }
    case AcyclicityType::NONE:
    {
        break;
    }
    default:
        ABORT("Unknown AcyclicityType");
    }
}

bool DeleteRelaxationRRConstraints::update_constraints(
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

class DeleteRelaxationRRConstraintsFeature
    : public plugins::TypedFeature<ConstraintGenerator,
                                   DeleteRelaxationRRConstraints> {
public:
    DeleteRelaxationRRConstraintsFeature()
        : TypedFeature("delete_relaxation_rr_constraints") {
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

        add_option<AcyclicityType>(
            "acyclicity_type",
            "The most relaxed version of this constraint only enforces that "
            "achievers of facts are picked in such a way that all goal facts "
            "have an achiever, and the preconditions all achievers are either "
            "true in the current state or have achievers themselves. In this "
            "version, cycles in the achiever relation can occur. Such cycles "
            "can be excluded with additional auxilliary varibles and "
            "constraints.",
            "vertex_elimination");
        add_option<bool>(
            "use_integer_vars",
            "restrict auxiliary variables to integer values. These variables "
            "encode whether facts are reached, which operator first achieves "
            "which fact, and (depending on the acyclicity_type) in which order "
            "the operators are used. Restricting them to integers generally "
            "improves the heuristic value at the cost of increased runtime.",
            "false");

        document_note(
            "Example",
            "To compute the optimal delete-relaxation heuristic h^+^, use"
            "integer variables and some way of enforcing acyclicity (other "
            "than \"none\"). For example\n"
            "{{{\noperatorcounting([delete_relaxation_rr_constraints("
            "acyclicity_type=vertex_elimination, use_integer_vars=true)], "
            "use_integer_operator_counts=true))\n}}}\n");
        document_note(
            "Note",
            "While the delete-relaxation constraints by Imai and Fukunaga "
            "(accessible via option {{{delete_relaxation_if_constraints}}}) "
            "serve a similar purpose to the constraints implemented here, we "
            "recommend using this formulation as it can generally be solved "
            "more efficiently, in particular in case of the h^+^ "
            "configuration, and some relaxations offer tighter bounds.\n");
    }
};

static plugins::FeaturePlugin<DeleteRelaxationRRConstraintsFeature> _plugin;

static plugins::TypedEnumPlugin<AcyclicityType> _enum_plugin({
        {"time_labels",
         "introduces MIP variables that encode the time at which each fact is "
         "reached. Acyclicity is enforced with constraints that ensure that "
         "preconditions of actions are reached before their effects."},
        {"vertex_elimination",
         "introduces binary variables based on vertex elimination. These "
         "variables encode that one fact has to be reached before another "
         "fact. Instead of adding such variables for every pair of states, "
         "they are only added for a subset sufficient to ensure acyclicity. "
         "Constraints enforce that preconditions of actions are reached before "
         "their effects and that the assignment encodes a valid order."},
        {"none",
         "No acyclicity is enforced. The resulting heuristic is a relaxation "
         "of the delete-relaxation heuristic."}
    });
}
