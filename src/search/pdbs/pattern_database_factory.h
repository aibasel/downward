#ifndef PDBS_PATTERN_DATABASE_FACTORY_H
#define PDBS_PATTERN_DATABASE_FACTORY_H

#include "types.h"

#include "../task_proxy.h"

#include <vector>

namespace pdbs {
class MatchTree;

class AbstractOperator {
    /*
      This class represents an abstract operator how it is needed for
      the regression search performed during the PDB-construction. As
      all abstract states are represented as a number, abstract
      operators don't have "usual" effects but "hash effects", i.e. the
      change (as number) the abstract operator implies on a given
      abstract state.
    */

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
    std::size_t hash_effect;
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
                     const std::vector<std::size_t> &hash_multipliers);
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
    std::size_t get_hash_effect() const {return hash_effect;}

    /*
      Returns the cost of the abstract operator (same as the cost of
      the original concrete operator)
    */
    int get_cost() const {return cost;}
    void dump(const Pattern &pattern,
              const VariablesProxy &variables) const;
};

/*
    Computes all abstract operators, builds the match tree (successor
    generator) and then does a Dijkstra regression search to compute
    all final h-values (stored in distances). operator_costs can
    specify individual operator costs for each operator for action
    cost partitioning. If left empty, default operator costs are used.
*/
class PatternDatabaseFactory {
    const TaskProxy &task_proxy;
    VariablesProxy variables;
    const Pattern &pattern;

    std::vector<size_t> hash_multipliers;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<AbstractOperator> operators;
    std::unique_ptr<MatchTree> match_tree;
    std::vector<FactPair> abstract_goals;
    std::vector<int> distances;

    void compute_hash_multipliers();
    void compute_variable_to_index();

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
        const std::vector<FactPair> &effects_without_pre);

    /*
      Computes all abstract operators for a given concrete operator (by
      its global operator number). Initializes data structures for initial
      call to recursive method multiply_out. variable_to_index maps
      variables in the task to their index in the pattern or -1.
    */
    void build_abstract_operators(
        const OperatorProxy &op, int cost);

    void compute_abstract_operators(
        const std::vector<int> &operator_costs);
    void build_match_tree();
    void compute_abstract_goals();

    /*
      For a given abstract state (given as index), the according values
      for each variable in the state are computed and compared with the
      given pairs of goal variables and values. Returns true iff the
      state is a goal state.
    */
    bool is_goal_state(size_t state_index) const;
    void compute_distances();

public:
    /*
      We recommend using generate_pdb() for creating PDBs.
      See there for a description of the parameters.
    */
    PatternDatabaseFactory(
        const TaskProxy &task_proxy,
        const Pattern &pattern,
        bool dump,
        const std::vector<int> &operator_costs);
    std::shared_ptr<PatternDatabase> extract_pdb();
};

/*
  Important: It is assumed that the given pattern is sorted, contains no
  duplicates and is small enough so that the number of abstract states is
  below numeric_limits<int>::max().
  Optional parameters:
   dump:           If set to true, prints the construction time.
   operator_costs: Can specify individual operator costs for each
   operator. This is useful for action cost partitioning. If left
   empty, default operator costs are used.
*/
extern std::shared_ptr<PatternDatabase> generate_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    bool dump = false,
    const std::vector<int> &operator_costs = std::vector<int>());
}

#endif
