#ifndef AXIOMS_H
#define AXIOMS_H

#include "packed_state_entry.h"

#include <vector>

class AxiomEvaluator {
    struct AxiomRule;
    struct AxiomLiteral {
        std::vector<AxiomRule *> condition_of;
    };
    struct AxiomRule {
        int condition_count;
        int unsatisfied_conditions;
        int effect_var;
        int effect_val;
        AxiomLiteral *effect_literal;
        AxiomRule(int cond_count, int eff_var, int eff_val, AxiomLiteral *eff_literal)
            : condition_count(cond_count), unsatisfied_conditions(cond_count),
              effect_var(eff_var), effect_val(eff_val), effect_literal(eff_literal) {
        }
    };
    struct NegationByFailureInfo {
        int var_no;
        AxiomLiteral *literal;
        NegationByFailureInfo(int var, AxiomLiteral *lit)
            : var_no(var), literal(lit) {}
    };

    std::vector<std::vector<AxiomLiteral> > axiom_literals;
    std::vector<AxiomRule> rules;
    std::vector<std::vector<NegationByFailureInfo> > nbf_info_by_layer;
public:
    AxiomEvaluator();
    void evaluate(PackedStateEntry *buffer);
};

#endif
