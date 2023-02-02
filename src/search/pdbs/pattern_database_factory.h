#ifndef PDBS_PATTERN_DATABASE_FACTORY_H
#define PDBS_PATTERN_DATABASE_FACTORY_H

#include "types.h"

#include "../task_proxy.h"

#include <tuple>
#include <utility>
#include <vector>

namespace utils {
class LogProxy;
class RandomNumberGenerator;
}

namespace pdbs {
class AbstractOperator {
    /*
      This class represents an abstract operator how it is needed for
      the regression search performed during the PDB-construction. As
      all abstract states are represented as a number, abstract
      operators don't have "usual" effects but "hash effects", i.e. the
      change (as number) the abstract operator implies on a given
      abstract state.
    */
    int concrete_op_id;

    int cost;

    /*
      Preconditions for the regression search, corresponds to normal
      effects and prevail of concrete operators.
    */
    std::vector<FactPair> regression_preconditions;

    /*
      Effect of the operator during regression search on a given
      abstract state number.
    */
    int hash_effect;
public:
    /*
      Abstract operators are built from concrete operators. The
      parameters follow the usual name convention of SAS+ operators,
      meaning prevail, preconditions and effects are all related to
      progression search.
    */
    AbstractOperator(const std::vector<FactPair> &prevail,
                     const std::vector<FactPair> &preconditions,
                     const std::vector<FactPair> &effects,
                     int cost,
                     const std::vector<int> &hash_multipliers,
                     int concrete_op_id);
    ~AbstractOperator();

    /*
      Returns variable value pairs which represent the preconditions of
      the abstract operator in a regression search
    */
    const std::vector<FactPair> &get_regression_preconditions() const {
        return regression_preconditions;
    }

    /*
      Returns the effect of the abstract operator in form of a value
      change (+ or -) to an abstract state index
    */
    int get_hash_effect() const {return hash_effect;}

    int get_concrete_op_id() const {
        return concrete_op_id;
    }

    /*
      Returns the cost of the abstract operator (same as the cost of
      the original concrete operator)
    */
    int get_cost() const {return cost;}
    void dump(const Pattern &pattern,
              const VariablesProxy &variables,
              utils::LogProxy &log) const;
};

class PatternDatabaseFactory {
    Pattern pattern;

    // size of the PDB
    int num_states;

    /*
      final h-values for abstract-states.
      dead-ends are represented by numeric_limits<int>::max()
    */
    std::vector<int> distances;

    std::vector<int> generating_op_ids;
    std::vector<std::vector<OperatorID>> wildcard_plan;

    // multipliers for each variable for perfect hash function
    std::vector<int> hash_multipliers;

    /*
      Recursive method; called by build_abstract_operators. In the case
      of a precondition with value = -1 in the concrete operator, all
      multiplied out abstract operators are computed, i.e. for all
      possible values of the variable (with precondition = -1), one
      abstract operator with a concrete value (!= -1) is computed.
    */
    void multiply_out(
        int pos, int cost,
        std::vector<FactPair> &prev_pairs,
        std::vector<FactPair> &pre_pairs,
        std::vector<FactPair> &eff_pairs,
        const std::vector<FactPair> &effects_without_pre,
        const VariablesProxy &variables,
        int concrete_op_id,
        std::vector<AbstractOperator> &operators);

    /*
      Computes all abstract operators for a given concrete operator (by
      its global operator number). Initializes data structures for initial
      call to recursive method multiply_out. variable_to_index maps
      variables in the task to their index in the pattern or -1.
    */
    void build_abstract_operators(
        const OperatorProxy &op, int cost,
        const std::vector<int> &variable_to_index,
        const VariablesProxy &variables,
        std::vector<AbstractOperator> &operators);

    /*
      Computes all abstract operators, builds the match tree (successor
      generator) and then does a Dijkstra regression search to compute
      all final h-values (stored in distances). operator_costs can
      specify individual operator costs for each operator for action
      cost partitioning. If left empty, default operator costs are used.
    */
    void create_pdb(
        const TaskProxy &task_proxy,
        const std::vector<int> &operator_costs,
        bool compute_plan,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        bool compute_wildcard_plan);

    /*
      For a given abstract state (given as index), the according values
      for each variable in the state are computed and compared with the
      given pairs of goal variables and values. Returns true iff the
      state is a goal state.
    */
    bool is_goal_state(
        int state_index,
        const std::vector<FactPair> &abstract_goals,
        const VariablesProxy &variables) const;

    /*
      The given concrete state is used to calculate the index of the
      according abstract state. This is only used for table lookup
      (distances) during search.
    */
    int hash_index(const std::vector<int> &state) const;
public:
    /*
      Important: It is assumed that the passed pattern is
      sorted, contains no duplicates and is small enough so that the
      number of abstract states is below numeric_limits<int>::max()
      Parameters:
       operator_costs: Can specify individual operator costs for each
       operator. This is useful for action cost partitioning. If left
       empty, default operator costs are used.
       compute_plan: if true, compute an optimal plan when computing
       distances of the PDB. This requires a RNG object passed via rng.
       compute_wildcard_plan: when computing a plan (see compute_plan), compute
       a wildcard plan, i.e., a sequence of parallel operators inducing an
       optimal plan. Otherwise, compute a simple plan (a sequence of operators).
    */
    PatternDatabaseFactory(
        const TaskProxy &task_proxy,
        const Pattern &pattern,
        const std::vector<int> &operator_costs = std::vector<int>(),
        bool compute_plan = false,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng = nullptr,
        bool compute_wildcard_plan = false);
    ~PatternDatabaseFactory() = default;

    std::shared_ptr<PatternDatabase> extract_pdb();

    // Returns the pattern (i.e. all variables used) of the PDB
    const Pattern &get_pattern() const {
        return pattern;
    }

    // Returns the size (number of abstract states) of the PDB
    int get_size() const {
        return num_states;
    }

    std::vector<std::vector<OperatorID>> && extract_wildcard_plan() {
        return std::move(wildcard_plan);
    };
};

/*
  Compute a PDB for the given task and pattern. If operator_costs is given,
  it should contain one integer for each operator of the task, specifying
  the cost that should be considered for each operator instead of the original
  cost of the operator.
*/
extern std::shared_ptr<PatternDatabase> compute_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const std::vector<int> &operator_costs = std::vector<int>(),
    const std::shared_ptr<utils::RandomNumberGenerator> &rng = nullptr);

/*
  In addition to computing a PDB for the given task and pattern like
  compute_pdb() above, also compute an abstract plan along.

  A wildcard plan is a sequence of set of operator IDs, where each such set
  induces the same transition between the two abstract states of this plan
  step.. That is, a wildcard plan induces a single trace of abstract states
  like a regular plan, but offers a set of "equivalent" operators for each
  plan step.

  If compute_wildcard_plan is false, each set of operator IDs of the returned
  plan contains exactly one operator, thus representing a regular plan. If
  set to true, each set contains at least one operator ID.
*/
extern std::tuple<std::shared_ptr<PatternDatabase>,
    std::vector<std::vector<OperatorID>>> compute_pdb_and_wildcard_plan(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const std::vector<int> &operator_costs = std::vector<int>(),
    const std::shared_ptr<utils::RandomNumberGenerator> &rng = nullptr,
    bool compute_wildcard_plan = false);
}

#endif
