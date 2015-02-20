#include "operator_cost.h"

#include "global_operator.h"
#include "global_task_interface.h"
#include "globals.h"
#include "option_parser.h"
#include "plugin.h"
#include "task.h"
#include "utilities.h"

#include <cstdlib>
#include <vector>
using namespace std;


int get_adjusted_action_cost(int cost, OperatorCost cost_type) {
    switch (cost_type) {
    case NORMAL:
        return cost;
    case ONE:
        return 1;
    case PLUSONE:
        if (is_unit_cost())
            return 1;
        else
            return cost + 1;
    default:
        ABORT("Unknown cost type");
    }
}

int get_adjusted_action_cost(const GlobalOperator &op, OperatorCost cost_type) {
    if (op.is_axiom())
        return 0;
    else
        return get_adjusted_action_cost(op.get_cost(), cost_type);
}


AdaptCosts::AdaptCosts(const TaskInterface &base_, const Options &opts)
    : base(base_),
      cost_type(OperatorCost(opts.get<int>("cost_type"))) {
}

AdaptCosts::~AdaptCosts() {
}

int AdaptCosts::get_num_variables() const {
    return base.get_num_variables();
}

int AdaptCosts::get_variable_domain_size(int var) const {
    return base.get_variable_domain_size(var);
}

int AdaptCosts::get_operator_cost(int index, bool is_axiom) const {
    return get_adjusted_action_cost(base.get_operator_cost(index, is_axiom), cost_type);
}

const std::string &AdaptCosts::get_operator_name(int index, bool is_axiom) const {
    return base.get_operator_name(index, is_axiom);
}

int AdaptCosts::get_num_operators() const {
    return base.get_num_operators();
}

int AdaptCosts::get_num_operator_preconditions(int index, bool is_axiom) const {
    return base.get_num_operator_preconditions(index, is_axiom);
}

pair<int, int> AdaptCosts::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return base.get_operator_precondition(op_index, fact_index, is_axiom);
}

int AdaptCosts::get_num_operator_effects(int op_index, bool is_axiom) const {
    return base.get_num_operator_effects(op_index, is_axiom);
}

int AdaptCosts::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return base.get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
}

pair<int, int> AdaptCosts::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return base.get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom);
}

pair<int, int> AdaptCosts::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return base.get_operator_effect(op_index, eff_index, is_axiom);
}

const GlobalOperator *AdaptCosts::get_global_operator(int index, bool is_axiom) const {
    return base.get_global_operator(index, is_axiom);
}

int AdaptCosts::get_num_axioms() const {
    return base.get_num_axioms();
}

int AdaptCosts::get_num_goals() const {
    return base.get_num_goals();
}

std::pair<int, int> AdaptCosts::get_goal_fact(int index) const {
    return base.get_goal_fact(index);
}


void add_cost_type_option_to_parser(OptionParser &parser) {
    vector<string> cost_types;
    vector<string> cost_types_doc;
    cost_types.push_back("NORMAL");
    cost_types_doc.push_back(
        "all actions are accounted for with their real cost");
    cost_types.push_back("ONE");
    cost_types_doc.push_back(
        "all actions are accounted for as unit cost");
    cost_types.push_back("PLUSONE");
    cost_types_doc.push_back(
        "all actions are accounted for as their real cost + 1 "
        "(except if all actions have original cost 1, "
        "in which case cost 1 is used). "
        "This is the behaviour known for the heuristics of the LAMA planner. "
        "This is intended to be used by the heuristics, not search engines, "
        "but is supported for both.");
    parser.add_enum_option(
        "cost_type",
        cost_types,
        "Operator cost adjustment type. "
        "No matter what this setting is, axioms will always be considered "
        "as actions of cost 0 by the heuristics that treat axioms as actions.",
        "NORMAL",
        cost_types_doc);
}


static Task *_parse(OptionParser &parser) {
    add_cost_type_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return 0;
    } else {
        GlobalTaskInterface *base = new GlobalTaskInterface();
        return new Task(new AdaptCosts(*base, opts));
    }
}

static Plugin<Task> _plugin("adapt_costs", _parse);
