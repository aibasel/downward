#include "cost_adapted_task.h"

#include "option_parser.h"
#include "plugin.h"

#include <string>
#include <vector>

using namespace std;


CostAdaptedTask::CostAdaptedTask(const Options &opts)
    : DelegatingTask(*opts.get<AbstractTask *>("transform")),
      cost_type(OperatorCost(opts.get<int>("cost_type"))) {
}

CostAdaptedTask::~CostAdaptedTask() {
}

int CostAdaptedTask::get_num_variables() const {
    return parent.get_num_variables();
}

int CostAdaptedTask::get_variable_domain_size(int var) const {
    return parent.get_variable_domain_size(var);
}

int CostAdaptedTask::get_operator_cost(int index, bool is_axiom) const {
    return get_adjusted_action_cost(parent.get_operator_cost(index, is_axiom), cost_type);
}

const string &CostAdaptedTask::get_operator_name(int index, bool is_axiom) const {
    return parent.get_operator_name(index, is_axiom);
}

int CostAdaptedTask::get_num_operators() const {
    return parent.get_num_operators();
}

int CostAdaptedTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return parent.get_num_operator_preconditions(index, is_axiom);
}

pair<int, int> CostAdaptedTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return parent.get_operator_precondition(op_index, fact_index, is_axiom);
}

int CostAdaptedTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return parent.get_num_operator_effects(op_index, is_axiom);
}

int CostAdaptedTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return parent.get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
}

pair<int, int> CostAdaptedTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return parent.get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom);
}

pair<int, int> CostAdaptedTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return parent.get_operator_effect(op_index, eff_index, is_axiom);
}

const GlobalOperator *CostAdaptedTask::get_global_operator(int index, bool is_axiom) const {
    return parent.get_global_operator(index, is_axiom);
}

int CostAdaptedTask::get_num_axioms() const {
    return parent.get_num_axioms();
}

int CostAdaptedTask::get_num_goals() const {
    return parent.get_num_goals();
}

pair<int, int> CostAdaptedTask::get_goal_fact(int index) const {
    return parent.get_goal_fact(index);
}

vector<int> CostAdaptedTask::get_state_values(const GlobalState &global_state) const {
    return parent.get_state_values(global_state);
}


static AbstractTask *_parse(OptionParser &parser) {
    parser.add_option<AbstractTask *>(
        "transform",
        "Parent task transformation",
        "no_transform");
    add_cost_type_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CostAdaptedTask(opts);
}

static Plugin<AbstractTask> _plugin("adapt_costs", _parse);
