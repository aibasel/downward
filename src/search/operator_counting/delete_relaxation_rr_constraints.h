#ifndef OPERATOR_COUNTING_DELETE_RELAXATION_RR_CONSTRAINTS_H
#define OPERATOR_COUNTING_DELETE_RELAXATION_RR_CONSTRAINTS_H

#include  "constraint_generator.h"

#include "../task_proxy.h"
#include "../utils/hash.h"

#include <memory>
#include <vector>

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

enum class AcyclicityType {
    TIME_LABELS, VERTEX_ELIMINATION, NONE
};

class DeleteRelaxationRRConstraints : public ConstraintGenerator {
    struct LPVariableIDs {
        /*
          The variables f_p in the paper represent if a fact p is reached by the
          relaxed plan encoded in the LP solution. We only store the offset for
          each variable (LP variables for different facts of the same variable
          are consecutive).
        */
        std::vector<int> fp_offsets;

        /*
          The variables f_{p,a} in the paper represent if an action a is used to
          achieve a fact p in the relaxed plan encoded in the LP solution. The
          variable is only needed for combinations of p and a where p is an
          effect of a. We store one hash map for each operator a that maps facts
          p to LP variable IDs.
        */
        std::vector<utils::HashMap<FactPair, int>> fpa_ids;

        /*
          The variable e_{i,j} in the paper is used as part of the vertex
          elimination method. It represents that fact p_i is used before fact
          p_j. Not all pairs of facts have to be ordered, the vertex elimination
          graph ensures that enough variables are created to exclude all cycles.
        */
        utils::HashMap<std::pair<FactPair, FactPair>, int> e_ids;

        /*
          The variable t_p in the paper is used as part of the time labels
          method. It represents the time at which fact p is first made true.
          We store the offsets here, analogous to fp.
        */
        std::vector<int> t_offsets;

        int id_of_fp(FactPair f) const;
        int id_of_fpa(FactPair f, const OperatorProxy &op) const;
        int id_of_e(std::pair<FactPair, FactPair> edge) const;
        int has_e(std::pair<FactPair, FactPair> edge) const;
        int id_of_t(FactPair f) const;
    };

    AcyclicityType acyclicity_type;
    bool use_integer_vars;

    /*
      Store offsets to identify Constraints (2) in the paper. We need to
      reference them when updating constraints for a given state. The constraint
      for a fact with variable v and value d has ID (constraint_offsets[v] + d).
    */
    std::vector<int> constraint_offsets;

    /* The state that is currently used for setting the bounds. Remembering
       this makes it faster to unset the bounds when the state changes. */
    std::vector<FactPair> last_state;


    int get_constraint_id(FactPair f) const;

    LPVariableIDs create_auxiliary_variables(
        const TaskProxy &task_proxy, LPVariables &variables) const;
    void create_auxiliary_variables_ve(
        const TaskProxy &task_proxy, const VEGraph &ve_graph,
        LPVariables &variables, LPVariableIDs &lp_var_ids) const;
    void create_auxiliary_variables_tl(
        const TaskProxy &task_proxy, LPVariables &variables,
        LPVariableIDs &lp_var_ids) const;
    void create_constraints(
        const TaskProxy &task_proxy, const LPVariableIDs &lp_var_ids,
        lp::LinearProgram &lp);
    void create_constraints_ve(
        const TaskProxy &task_proxy, const VEGraph &ve_graph,
        const LPVariableIDs &lp_var_ids, lp::LinearProgram &lp);
    void create_constraints_tl(
        const TaskProxy &task_proxy, const LPVariableIDs &lp_var_ids,
        lp::LinearProgram &lp);
public:
    explicit DeleteRelaxationRRConstraints(const plugins::Options &opts);

    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> &task,
        lp::LinearProgram &lp) override;
    virtual bool update_constraints(
        const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif
