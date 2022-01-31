#ifndef AXIOMS_H
#define AXIOMS_H

#include "per_task_information.h"
#include "task_proxy.h"

#include <memory>
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

    bool task_has_axioms;

    std::vector<std::vector<AxiomLiteral>> axiom_literals;
    std::vector<AxiomRule> rules;
    std::vector<std::vector<NegationByFailureInfo>> nbf_info_by_layer;
    /*
      default_values stores the default (negation by failure) values
      for all derived variables, i.e., the value that a derived
      variable holds by default if no derivation rule triggers.

      This is indexed by variable number and set to -1 for non-derived
      variables, so can also be used to test if a variable is derived.

      We have our own copy of the data to avoid going through the task
      interface in the time-critical evaluate method.
    */
    std::vector<int> default_values;

    /*
      The queue is an instance variable rather than a local variable
      to reduce reallocation effort. See issue420.
    */
    std::vector<const AxiomLiteral *> queue;

    template<typename Values, typename Accessor>
    void evaluate_aux(Values &values, const Accessor &accessor);
public:
    explicit AxiomEvaluator(const TaskProxy &task_proxy);

    void evaluate(std::vector<int> &state);
};

extern PerTaskInformation<AxiomEvaluator> g_axiom_evaluators;

#endif
