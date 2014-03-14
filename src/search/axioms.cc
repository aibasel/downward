#include "axioms.h"
#include "globals.h"
#include "int_packer.h"
#include "operator.h"

#include <iostream>
#include <vector>
using namespace std;

AxiomEvaluator::AxiomEvaluator() {
    // Initialize literals
    for (int i = 0; i < g_variable_domain.size(); i++)
        axiom_literals.push_back(vector<AxiomLiteral>(g_variable_domain[i]));

    // Initialize rules
    for (int i = 0; i < g_axioms.size(); i++) {
        const Operator &axiom = g_axioms[i];
        int cond_count = axiom.get_pre_post()[0].cond.size();
        int eff_var = axiom.get_pre_post()[0].var;
        int eff_val = axiom.get_pre_post()[0].post;
        AxiomLiteral *eff_literal = &axiom_literals[eff_var][eff_val];
        rules.push_back(AxiomRule(cond_count, eff_var, eff_val, eff_literal));
    }

    // Cross-reference rules and literals
    for (int i = 0; i < g_axioms.size(); i++) {
        const vector<Prevail> &conditions = g_axioms[i].get_pre_post()[0].cond;
        for (int j = 0; j < conditions.size(); j++) {
            const Prevail &cond = conditions[j];
            axiom_literals[cond.var][cond.prev].condition_of.push_back(&rules[i]);
        }
    }

    // Initialize negation-by-failure information
    int last_layer = -1;
    for (int i = 0; i < g_axiom_layers.size(); i++)
        last_layer = max(last_layer, g_axiom_layers[i]);
    nbf_info_by_layer.resize(last_layer + 1);

    for (int var_no = 0; var_no < g_axiom_layers.size(); var_no++) {
        int layer = g_axiom_layers[var_no];
        if (layer != -1 && layer != last_layer) {
            int nbf_value = g_default_axiom_values[var_no];
            AxiomLiteral *nbf_literal = &axiom_literals[var_no][nbf_value];
            NegationByFailureInfo nbf_info(var_no, nbf_literal);
            nbf_info_by_layer[layer].push_back(nbf_info);
        }
    }
}

// TODO rethink the way this is called: see issue348.
void AxiomEvaluator::evaluate(PackedStateBin *buffer) {
    if (!has_axioms())
        return;

    assert(queue.empty());
    for (int i = 0; i < g_axiom_layers.size(); i++) {
        if (g_axiom_layers[i] != -1) {
            g_state_packer->set(buffer, i, g_default_axiom_values[i]);
        } else {
            queue.push_back(&axiom_literals[i][g_state_packer->get(buffer, i)]);
        }
    }

    for (int i = 0; i < rules.size(); i++) {
        rules[i].unsatisfied_conditions = rules[i].condition_count;

        // TODO: In a perfect world, trivial axioms would have been
        // compiled away, and we could have the following assertion
        // instead of the following block.
        // assert(rules[i].condition_counter != 0);
        if (rules[i].condition_count == 0) {
            // NOTE: This duplicates code from the main loop below.
            // I don't mind because this is (hopefully!) going away
            // some time.
            int var_no = rules[i].effect_var;
            int val = rules[i].effect_val;
            if (g_state_packer->get(buffer, var_no) != val) {
                g_state_packer->set(buffer, var_no, val);
                queue.push_back(rules[i].effect_literal);
            }
        }
    }

    for (int layer_no = 0; layer_no < nbf_info_by_layer.size(); layer_no++) {
        // Apply Horn rules.
        while (!queue.empty()) {
            AxiomLiteral *curr_literal = queue.back();
            queue.pop_back();
            for (int i = 0; i < curr_literal->condition_of.size(); i++) {
                AxiomRule *rule = curr_literal->condition_of[i];
                if (--(rule->unsatisfied_conditions) == 0) {
                    int var_no = rule->effect_var;
                    int val = rule->effect_val;
                    if (g_state_packer->get(buffer, var_no) != val) {
                        g_state_packer->set(buffer, var_no, val);
                        queue.push_back(rule->effect_literal);
                    }
                }
            }
        }

        // Apply negation by failure rules. Skip this in last iteration
        // to save some time (see issue420, msg3058).
        if (layer_no != nbf_info_by_layer.size() - 1) {
            const vector<NegationByFailureInfo> &nbf_info = nbf_info_by_layer[layer_no];
            for (int i = 0; i < nbf_info.size(); i++) {
                int var_no = nbf_info[i].var_no;
                if (g_state_packer->get(buffer, var_no) == g_default_axiom_values[var_no])
                    queue.push_back(nbf_info[i].literal);
            }
        }
    }
}
