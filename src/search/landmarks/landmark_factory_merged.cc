#include "landmark_factory_merged.h"

#include "exploration.h"
#include "landmark_graph.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

#include <set>

using namespace std;
using utils::ExitCode;

namespace landmarks {
class LandmarkNode;

LandmarkFactoryMerged::LandmarkFactoryMerged(const Options &opts)
    : LandmarkFactory(opts),
      lm_factories(opts.get_list<shared_ptr<LandmarkFactory>>("lm_factories")) {
}

LandmarkNode *LandmarkFactoryMerged::get_matching_landmark(const LandmarkNode &lm) const {
    if (!lm.disjunctive && !lm.conjunctive) {
        const FactPair &lm_fact = lm.facts[0];
        if (lm_graph->simple_landmark_exists(lm_fact))
            return &lm_graph->get_simple_lm_node(lm_fact);
        else
            return 0;
    } else if (lm.disjunctive) {
        set<FactPair> lm_facts(lm.facts.begin(), lm.facts.end());
        if (lm_graph->exact_same_disj_landmark_exists(lm_facts))
            return &lm_graph->get_disj_lm_node(lm.facts[0]);
        else
            return 0;
    } else if (lm.conjunctive) {
        cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
    return 0;
}

void LandmarkFactoryMerged::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    utils::g_log << "Merging " << lm_factories.size() << " landmark graphs" << endl;

    std::vector<std::shared_ptr<LandmarkGraph>> lm_graphs;
    lm_graphs.reserve(lm_factories.size());
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        lm_graphs.push_back(lm_factory->compute_lm_graph(task));
    }

    utils::g_log << "Adding simple landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const LandmarkGraph::Nodes &nodes = lm_graphs[i]->get_nodes();
        for (auto &lm : nodes) {
            const LandmarkNode &node = *lm;
            const FactPair &lm_fact = node.facts[0];
            if (!node.conjunctive && !node.disjunctive && !lm_graph->landmark_exists(lm_fact)) {
                LandmarkNode &new_node = lm_graph->landmark_add_simple(lm_fact);
                new_node.in_goal = node.in_goal;
            }
        }
    }

    // TODO: It seems that disjunctive landmarks are only added if none of its
    //  facts is also there as a simple landmark. This should either be more
    //  general (no subset is there as a disjunctive landmark) or be removed at
    //  all (if orders are not considered, these can be removed).

    utils::g_log << "Adding disjunctive landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const LandmarkGraph::Nodes &nodes = lm_graphs[i]->get_nodes();
        for (auto &lm : nodes) {
            const LandmarkNode &node = *lm;
            if (node.disjunctive) {
                set<FactPair> lm_facts;
                bool exists = false;
                for (const FactPair &lm_fact: node.facts) {
                    if (lm_graph->landmark_exists(lm_fact)) {
                        exists = true;
                        break;
                    }
                    lm_facts.insert(lm_fact);
                }
                if (!exists) {
                    LandmarkNode &new_node = lm_graph->landmark_add_disjunctive(lm_facts);
                    new_node.in_goal = node.in_goal;
                }
            } else if (node.conjunctive) {
                cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
                utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
            }
        }
    }

    utils::g_log << "Adding orderings" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const LandmarkGraph::Nodes &nodes = lm_graphs[i]->get_nodes();
        for (auto &from_orig : nodes) {
            LandmarkNode *from = get_matching_landmark(*from_orig);
            if (from) {
                for (const auto &to : from_orig->children) {
                    const LandmarkNode &to_orig = *to.first;
                    EdgeType e_type = to.second;
                    LandmarkNode *to_node = get_matching_landmark(to_orig);
                    if (to_node) {
                        edge_add(*from, *to_node, e_type);
                    } else {
                        utils::g_log << "Discarded to ordering" << endl;
                    }
                }
            } else {
                utils::g_log << "Discarded from ordering" << endl;
            }
        }
    }

    TaskProxy task_proxy(*task);
    generate(task_proxy);
}

void LandmarkFactoryMerged::generate(const TaskProxy &task_proxy) {
    lm_graph->set_landmark_ids();

    // TODO: causal, disjunctive and/or conjunctive landmarks as well as orders
    //  have been removed in the individual landmark graphs. Since merging
    //  landmark graphs doesn't introduce any of these, it should not be
    //  necessary to do so again here, so these steps are omitted. For
    //  reasonable orders, acyclicity of the landmark graph, the costs of
    //  landmarks and the achievers we should also determine this.
    if (reasonable_orders) {
        utils::g_log << "approx. reasonable orders" << endl;
        approximate_reasonable_orders(task_proxy, false);
        utils::g_log << "approx. obedient reasonable orders" << endl;
        approximate_reasonable_orders(task_proxy, true);
    }
    mk_acyclic_graph();
    lm_graph->set_landmark_cost(calculate_lms_cost());

    // TODO: We should copy the achievers of each landmark when the individual
    //  landmark graphs are merged. That way, we do not have to recompute them
    //  here and get rid of the Exploration object (as well as the the methods
    //  calc_achievers, relaxed_task_solvable, achieves_non_conditional and
    //  add_operator_and_propositions_to_list).
    calc_achievers(task_proxy);
}

void LandmarkFactoryMerged::calc_achievers(const TaskProxy &task_proxy) {
    Exploration exploration(task_proxy);
    VariablesProxy variables = task_proxy.get_variables();
    for (auto &lmn : lm_graph->get_nodes()) {
        for (const FactPair &lm_fact : lmn->facts) {
            const vector<int> &ops = lm_graph->get_operators_including_eff(lm_fact);
            lmn->possible_achievers.insert(ops.begin(), ops.end());

            if (variables[lm_fact.var].is_derived())
                lmn->is_derived = true;
        }

        vector<vector<int>> lvl_var;
        vector<utils::HashMap<FactPair, int>> lvl_op;
        relaxed_task_solvable(task_proxy, exploration, lvl_var, lvl_op, true, lmn.get());

        for (int op_or_axom_id : lmn->possible_achievers) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axom_id);

            if (_possibly_reaches_lm(op, lvl_var, lmn.get())) {
                lmn->first_achievers.insert(op_or_axom_id);
            }
        }
    }
}

bool LandmarkFactoryMerged::relaxed_task_solvable(
    const TaskProxy &task_proxy, Exploration &exploration,
    vector<vector<int>> &lvl_var, vector<utils::HashMap<FactPair, int>> &lvl_op,
    bool level_out, const LandmarkNode *exclude, bool compute_lvl_op) const {
    /* Test whether the relaxed planning task is solvable without achieving the propositions in
     "exclude" (do not apply operators that would add a proposition from "exclude").
     As a side effect, collect in lvl_var and lvl_op the earliest possible point in time
     when a proposition / operator can be achieved / become applicable in the relaxed task.
     */

    OperatorsProxy operators = task_proxy.get_operators();
    AxiomsProxy axioms = task_proxy.get_axioms();
    // Initialize lvl_op and lvl_var to numeric_limits<int>::max()
    if (compute_lvl_op) {
        lvl_op.resize(operators.size() + axioms.size());
        for (OperatorProxy op : operators) {
            add_operator_and_propositions_to_list(op, lvl_op);
        }
        for (OperatorProxy axiom : axioms) {
            add_operator_and_propositions_to_list(axiom, lvl_op);
        }
    }
    VariablesProxy variables = task_proxy.get_variables();
    lvl_var.resize(variables.size());
    for (VariableProxy var : variables) {
        lvl_var[var.get_id()].resize(var.get_domain_size(),
                                     numeric_limits<int>::max());
    }
    // Extract propositions from "exclude"
    unordered_set<int> exclude_op_ids;
    vector<FactPair> exclude_props;
    if (exclude) {
        for (OperatorProxy op : operators) {
            if (achieves_non_conditional(op, exclude))
                exclude_op_ids.insert(op.get_id());
        }
        exclude_props.insert(exclude_props.end(),
                             exclude->facts.begin(), exclude->facts.end());
    }
    // Do relaxed exploration
    exploration.compute_reachability_with_excludes(
        lvl_var, lvl_op, level_out, exclude_props, exclude_op_ids, compute_lvl_op);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (FactProxy goal : task_proxy.get_goals())
        if (lvl_var[goal.get_variable().get_id()][goal.get_value()] ==
            numeric_limits<int>::max())
            return false;

    return true;
}

bool LandmarkFactoryMerged::achieves_non_conditional(
    const OperatorProxy &o, const LandmarkNode *lmp) const {
    /* Test whether the landmark is achieved by the operator unconditionally.
    A disjunctive landmark is achieved if one of its disjuncts is achieved. */
    assert(lmp);
    for (EffectProxy effect: o.get_effects()) {
        for (const FactPair &lm_fact : lmp->facts) {
            FactProxy effect_fact = effect.get_fact();
            if (effect_fact.get_pair() == lm_fact) {
                if (effect.get_conditions().empty())
                    return true;
            }
        }
    }
    return false;
}

void LandmarkFactoryMerged::add_operator_and_propositions_to_list(
    const OperatorProxy &op, vector<utils::HashMap<FactPair, int>> &lvl_op) const {
    int op_or_axiom_id = get_operator_or_axiom_id(op);
    for (EffectProxy effect : op.get_effects()) {
        lvl_op[op_or_axiom_id].emplace(effect.get_fact().get_pair(), numeric_limits<int>::max());
    }
}

bool LandmarkFactoryMerged::supports_conditional_effects() const {
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        if (!lm_factory->supports_conditional_effects()) {
            return false;
        }
    }
    return true;
}

static shared_ptr<LandmarkFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Merged Landmarks",
        "Merges the landmarks and orderings from the parameter landmarks");
    parser.document_note(
        "Precedence",
        "Fact landmarks take precedence over disjunctive landmarks, "
        "orderings take precedence in the usual manner "
        "(gn > nat > reas > o_reas). ");
    parser.document_note(
        "Relevant options",
        "Depends on landmarks");
    parser.document_note(
        "Note",
        "Does not currently support conjunctive landmarks");
    parser.add_list_option<shared_ptr<LandmarkFactory>>("lm_factories");
    _add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<shared_ptr<LandmarkFactory>>("lm_factories");

    parser.document_language_support("conditional_effects",
                                     "supported if all components support them");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkFactoryMerged>(opts);
}

static Plugin<LandmarkFactory> _plugin("lm_merged", _parse);
}
