#ifndef OPERATOR_COUNTING_DELETE_RELAXATION_CONSTRAINTS_RR_H
#define OPERATOR_COUNTING_DELETE_RELAXATION_CONSTRAINTS_RR_H

#include  "constraint_generator.h"

#include "../task_proxy.h"
#include "../utils/hash.h"

#include <memory>

namespace lp {
class LPConstraint;
struct LPVariable;
}

namespace plugins {
class Options;
}

namespace operator_counting {
class VEGraph;
using LPConstraints = named_vector::NamedVector<lp::LPConstraint>;
using LPVariables = named_vector::NamedVector<lp::LPVariable>;

class DeleteRelaxationConstraintsRR : public ConstraintGenerator {
    // TODO: Rework these.
    bool use_time_vars;
    bool use_integer_vars;

    /*
      A causal partial function f maps a fact to one of its achieving operators.
    */

    /*
      [f_p] Is f defined for fact p?
      Binary, indexed with var.id, value */
    std::vector<std::vector<int>> lp_var_id_f_defined;

    /*
      [f_{p,a}] Does f map fact p to operator a?
      Binary, maps <var.id, value, operator.id> to LP variable index */
    //std::unordered_map<std::tuple<int, int, int>, int> lp_var_id_f_maps_to;
    utils::HashMap<std::tuple<int, int, int>, int> lp_var_id_f_maps_to;

    utils::HashMap<std::pair<FactPair, FactPair>, int> lp_var_id_edge;

    /*
      Store constraint IDs of Constraints (2) in the paper. We need to
      reference them when updating constraints for a given state. They are
      indexed by the fact p.
    */
    std::vector<std::vector<int>> lp_con_id_f_defined;

    /* The state that is currently used for setting the bounds. Remembering
       this makes it faster to unset the bounds when the state changes. */
    std::vector<FactPair> last_state;


    int get_var_f_defined(FactPair f);
    int get_var_f_maps_to(FactPair f, const OperatorProxy &op);
    int get_constraint_id(FactPair f);
    bool is_in_effect(FactPair f, const OperatorProxy &op);
    bool is_in_precondition(FactPair f, const OperatorProxy &op);

    void create_auxiliary_variables(
        const TaskProxy &task_proxy, LPVariables &variables, const VEGraph &ve_graph);
    void create_constraints(const TaskProxy &task_proxy, lp::LinearProgram &lp, const VEGraph &ve_graph);
public:
    explicit DeleteRelaxationConstraintsRR(const plugins::Options &opts);

    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> &task,
        lp::LinearProgram &lp) override;
    virtual bool update_constraints(
        const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif
