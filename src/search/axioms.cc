#include "axioms.h"

#include "algorithms/int_packer.h"
#include "task_utils/task_properties.h"
#include "utils/memory.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

/*
  We use a template-based version of the strategy pattern to support both packed
  and non-packed states (more precisely, int vectors) within the axiom
  evaluator. This is encapsulated in the following two accessor classes.

  As of this writing (i.e., if this comment is not out of date),
  PackedStateAccessor is not actually needed because the IntPacker itself
  already has the correct interface. However, having an explicit accessor helps
  decouple this code from the interface of IntPacker.
*/

class VectorStateAccessor {
public:
    void set(vector<int> &values, int var, int value) const {
        values[var] = value;
    }

    int get(const vector<int> &values, int var) const {
        return values[var];
    }
};

class PackedStateAccessor {
    const int_packer::IntPacker &state_packer;
public:
    explicit PackedStateAccessor(const int_packer::IntPacker &state_packer)
        : state_packer(state_packer) {
    }

    void set(PackedStateBin *buffer, int var, int value) const {
        state_packer.set(buffer, var, value);
    }

    int get(const PackedStateBin *buffer, int var) const {
        return state_packer.get(buffer, var);
    }
};

AxiomEvaluator::AxiomEvaluator(const TaskProxy &task_proxy) {
    task_has_axioms = task_properties::has_axioms(task_proxy);
    if (task_has_axioms) {
        VariablesProxy variables = task_proxy.get_variables();
        AxiomsProxy axioms = task_proxy.get_axioms();

        // Initialize literals
        for (VariableProxy var : variables)
            axiom_literals.emplace_back(var.get_domain_size());

        // Initialize rules
        for (OperatorProxy axiom : axioms) {
            assert(axiom.get_effects().size() == 1);
            EffectProxy cond_effect = axiom.get_effects()[0];
            FactPair effect = cond_effect.get_fact().get_pair();
            int num_conditions = cond_effect.get_conditions().size();
            AxiomLiteral *eff_literal = &axiom_literals[effect.var][effect.value];
            rules.emplace_back(
                num_conditions, effect.var, effect.value, eff_literal);
        }

        // Cross-reference rules and literals
        for (OperatorProxy axiom : axioms) {
            EffectProxy effect = axiom.get_effects()[0];
            for (FactProxy condition : effect.get_conditions()) {
                int var_id = condition.get_variable().get_id();
                int val = condition.get_value();
                AxiomRule *rule = &rules[axiom.get_id()];
                axiom_literals[var_id][val].condition_of.push_back(rule);
            }
        }

        // Initialize negation-by-failure information
        int last_layer = -1;
        for (VariableProxy var : variables) {
            if (var.is_derived()) {
                last_layer = max(last_layer, var.get_axiom_layer());
            }
        }
        nbf_info_by_layer.resize(last_layer + 1);

        for (VariableProxy var : variables) {
            if (var.is_derived()) {
                int layer = var.get_axiom_layer();
                if (layer != last_layer) {
                    int var_id = var.get_id();
                    int nbf_value = var.get_default_axiom_value();
                    AxiomLiteral *nbf_literal = &axiom_literals[var_id][nbf_value];
                    nbf_info_by_layer[layer].emplace_back(var_id, nbf_literal);
                }
            }
        }

        default_values.reserve(variables.size());
        for (VariableProxy var : variables) {
            if (var.is_derived())
                default_values.emplace_back(var.get_default_axiom_value());
            else
                default_values.emplace_back(-1);
        }
    }
}

// TODO rethink the way this is called: see issue348.
void AxiomEvaluator::evaluate(vector<int> &state) {
    evaluate_aux(state, VectorStateAccessor());
}

void AxiomEvaluator::evaluate(PackedStateBin *buffer,
                              const int_packer::IntPacker &state_packer) {
    evaluate_aux(buffer, PackedStateAccessor(state_packer));
}

template<typename Values, typename Accessor>
inline void AxiomEvaluator::evaluate_aux(Values &values, const Accessor &accessor) {
    if (!task_has_axioms)
        return;

    assert(queue.empty());
    for (size_t var_id = 0; var_id < default_values.size(); ++var_id) {
        int default_value = default_values[var_id];
        if (default_value != -1) {
            accessor.set(values, var_id, default_value);
        } else {
            int value = accessor.get(values, var_id);
            queue.push_back(&axiom_literals[var_id][value]);
        }
    }

    for (AxiomRule &rule : rules) {
        rule.unsatisfied_conditions = rule.condition_count;

        /*
          TODO: In a perfect world, trivial axioms would have been
          compiled away, and we could have the following assertion
          instead of the following block.
          assert(rule.condition_count != 0);
        */
        if (rule.condition_count == 0) {
            /*
              NOTE: This duplicates code from the main loop below.
              I don't mind because this is (hopefully!) going away
              some time.
            */
            int var_no = rule.effect_var;
            int val = rule.effect_val;
            if (accessor.get(values, var_no) != val) {
                accessor.set(values, var_no, val);
                queue.push_back(rule.effect_literal);
            }
        }
    }

    for (size_t layer_no = 0; layer_no < nbf_info_by_layer.size(); ++layer_no) {
        // Apply Horn rules.
        while (!queue.empty()) {
            const AxiomLiteral *curr_literal = queue.back();
            queue.pop_back();
            for (size_t i = 0; i < curr_literal->condition_of.size(); ++i) {
                AxiomRule *rule = curr_literal->condition_of[i];
                if (--rule->unsatisfied_conditions == 0) {
                    int var_no = rule->effect_var;
                    int val = rule->effect_val;
                    if (accessor.get(values, var_no) != val) {
                        accessor.set(values, var_no, val);
                        queue.push_back(rule->effect_literal);
                    }
                }
            }
        }

        /*
          Apply negation by failure rules. Skip this in last iteration
          to save some time (see issue420, msg3058).
        */
        if (layer_no != nbf_info_by_layer.size() - 1) {
            const vector<NegationByFailureInfo> &nbf_info = nbf_info_by_layer[layer_no];
            for (size_t i = 0; i < nbf_info.size(); ++i) {
                int var_no = nbf_info[i].var_no;
                // Verify that variable is derived.
                assert(default_values[var_no] != -1);
                if (accessor.get(values, var_no) == default_values[var_no])
                    queue.push_back(nbf_info[i].literal);
            }
        }
    }
}

PerTaskInformation<AxiomEvaluator> g_axiom_evaluators;
