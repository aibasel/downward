#include "projected_task.h"

#include "../task_proxy.h"
#include "../task_utils/task_properties.h"
#include <algorithm>

namespace tasks {
ProjectedTask::ProjectedTask(
    const std::shared_ptr<AbstractTask> &parent,
    const pdbs::Pattern &pattern)
    : DelegatingTask(parent),
      pattern(pattern) {
    TaskProxy parent_proxy(*parent);
    assert(!task_properties::has_axioms(parent_proxy));

    // remember where in the pattern each variable is stored
    for (size_t i = 0; i < pattern.size(); ++i) {
        int var = pattern[i];
        var_to_index[var] = i;
    }

    // make a list of operator indices for operators which are
    // relevant to at least one variable in the pattern
    for (int opi = 0; opi < parent->get_num_operators(); ++opi) {
        std::vector<FactPair> effects;

        for (int effi = 0;
             effi < parent->get_num_operator_effects(opi, false); ++effi) {
            FactPair effect =
                parent->get_operator_effect(opi, effi, false);
            int var = effect.var;

            if (var_to_index.count(var) != 0) {
                effects.push_back(convert_from_original_fact(effect));
            }
        }

        if (effects.empty()) {
            continue;
        } else {
            operator_indices.push_back(opi);
            operator_effects.push_back(effects);
        }

        std::vector<FactPair> preconditions;
        for (int condi = 0;
             condi < parent->get_num_operator_preconditions(opi, false); ++condi) {
            FactPair precondition =
                parent->get_operator_precondition(opi, condi, false);
            int var = precondition.var;

            // check if the precondition variable appears in the pattern
            if (var_to_index.count(var) != 0) {
                preconditions.push_back(convert_from_original_fact(precondition));
            }
        }
        operator_preconditions.push_back(preconditions);
    }

    for (int goali = 0;
         goali < parent->get_num_goals(); ++goali) {
        FactPair goal =
            parent->get_goal_fact(goali);
        int var = goal.var;

        auto res = std::find(std::begin(pattern), std::end(pattern), var);
        if (res != std::end(pattern)) {
            goals.push_back(convert_from_original_fact(goal));
        }
    }
    assert((unsigned)parent->get_num_goals() >= goals.size());
}

int ProjectedTask::get_original_variable_index(int index_in_pattern) const {
    assert(index_in_pattern >= 0 &&
           static_cast<unsigned>(index_in_pattern) < pattern.size());
    return pattern[index_in_pattern];
}

int ProjectedTask::get_pattern_variable_index(int index_in_original) const {
    auto it = var_to_index.find(index_in_original);
    if (it != var_to_index.end()) {
        return it->second;
    } else {
        std::cout << "ProjectedTask: "
                  << "A function tried to access a variable that is not part of the pattern."
                  << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

FactPair ProjectedTask::convert_from_pattern_fact(const FactPair &fact) const {
    return {
               get_original_variable_index(fact.var), fact.value
    };
}

FactPair ProjectedTask::convert_from_original_fact(const FactPair &fact) const {
    return {
               get_pattern_variable_index(fact.var), fact.value
    };
}

int ProjectedTask::get_num_variables() const {
    return pattern.size();
}

std::string ProjectedTask::get_variable_name(int var) const {
    int index = get_original_variable_index(var);
    return parent->get_variable_name(index);
}

int ProjectedTask::get_variable_domain_size(int var) const {
    int index = get_original_variable_index(var);
    return parent->get_variable_domain_size(index);
}

int ProjectedTask::get_variable_axiom_layer(int var) const {
    int index = get_original_variable_index(var);
    return parent->get_variable_axiom_layer(index);
}

int ProjectedTask::get_variable_default_axiom_value(int var) const {
    int index = get_original_variable_index(var);
    return parent->get_variable_default_axiom_value(index);
}

std::string ProjectedTask::get_fact_name(const FactPair &fact) const {
    return parent->get_fact_name(convert_from_pattern_fact(fact));
}

bool ProjectedTask::are_facts_mutex(
    const FactPair &fact1, const FactPair &fact2) const {
    return parent->are_facts_mutex(convert_from_pattern_fact(fact1),
                                   convert_from_pattern_fact(fact2));
}

int ProjectedTask::get_operator_cost(int index, bool is_axiom) const {
    assert(!is_axiom);
    index = convert_operator_index_to_parent(index);
    return parent->get_operator_cost(index, is_axiom);
}

std::string ProjectedTask::get_operator_name(int index, bool is_axiom) const {
    assert(!is_axiom);
    index = convert_operator_index_to_parent(index);
    return parent->get_operator_name(index, is_axiom);
}

int ProjectedTask::get_num_operators() const {
    return operator_indices.size();
}

int ProjectedTask::get_num_operator_preconditions(
    int index, bool) const {
    return operator_preconditions[index].size();
}

FactPair ProjectedTask::get_operator_precondition(
    int op_index, int fact_index, bool) const {
    return operator_preconditions[op_index][fact_index];
}

int ProjectedTask::get_num_operator_effects(
    int op_index, bool) const {
    return operator_effects[op_index].size();
}

int ProjectedTask::get_num_operator_effect_conditions(
    int, int, bool) const {
    return 0;
}

FactPair ProjectedTask::get_operator_effect_condition(
    int, int, int, bool) const {
    std::cerr << "get_operator_effect_condition is not supported yet." << std::endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

FactPair ProjectedTask::get_operator_effect(
    int op_index, int eff_index, bool) const {
    return operator_effects[op_index][eff_index];
}

int ProjectedTask::convert_operator_index_to_parent(int index) const {
    assert(index >= 0 && static_cast<unsigned>(index) < operator_indices.size());
    return operator_indices[index];
}

int ProjectedTask::get_num_goals() const {
    return goals.size();
}

FactPair ProjectedTask::get_goal_fact(int index) const {
    return goals[index];
}

std::vector<int> ProjectedTask::get_initial_state_values() const {
    std::vector<int> values = parent->get_initial_state_values();
    convert_parent_state_values(values);
    return values;
}

void ProjectedTask::convert_parent_state_values(
    std::vector<int> &values) const {
    std::vector<int> converted;
    for (int index : pattern) {
        converted.push_back(values[index]);
    }
    values.swap(converted);
}
}
