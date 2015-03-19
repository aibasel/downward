#ifndef PDBS_PDB_HEURISTIC_H
#define PDBS_PDB_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

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
    std::vector<std::pair<int, int> > regression_preconditions;

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
    AbstractOperator(const std::vector<std::pair<int, int> > &prevail,
                     const std::vector<std::pair<int, int> > &preconditions,
                     const std::vector<std::pair<int, int> > &effects, int cost,
                     const std::vector<std::size_t> &hash_multipliers);
    ~AbstractOperator();

    /*
      Returns variable value pairs which represent the preconditions of
      the abstract operator in a regression search
    */
    const std::vector<std::pair<int, int> > &get_regression_preconditions() const {
        return regression_preconditions;
    }

    /*
      Returns the effect of the abstract operator in form of a value
      change (+ or -) to an abstract state index
    */
    std::size_t get_hash_effect() const {return hash_effect; }

    /*
      Returns the cost of the abstract operator (same as the cost of
      the original concrete operator)
    */
    int get_cost() const {return cost; }
    void dump(const std::vector<int> &pattern) const;
};

// Implements a single PDB
class GlobalOperator;
class GlobalState;
class PDBHeuristic : public Heuristic {
    std::vector<int> pattern;

    // size of the PDB
    std::size_t num_states;

    /*
      final h-values for abstract-states.
      dead-ends are represented by numeric_limits<int>::max()
    */
    std::vector<int> distances;

    // multipliers for each variable for perfect hash function
    std::vector<std::size_t> hash_multipliers;

    /*
      Recursive method; called by build_abstract_operators. In the case
      of a precondition with value = -1 in the concrete operator, all
      multiplied out abstract operators are computed, i.e. for all
      possible values of the variable (with precondition = -1), one
      abstract operator with a concrete value (!= -1) is computed.
    */
    void multiply_out(
        int pos, int cost,
        std::vector<std::pair<int, int> > &prev_pairs,
        std::vector<std::pair<int, int> > &pre_pairs,
        std::vector<std::pair<int, int> > &eff_pairs,
        const std::vector<std::pair<int, int> > &effects_without_pre,
        std::vector<AbstractOperator> &operators);

    /*
      Computes all abstract operators for a given concrete operator (by
      its global operator number). Initializes datastructures for initial
      call to recursive method multiyply_out. variable_to_index maps
      variables in the task to their index in the pattern or -1.
    */
    void build_abstract_operators(
        const GlobalOperator &op, int cost,
        const std::vector<int> &variable_to_index,
        std::vector<AbstractOperator> &operators);

    /*
      Computes all abstract operators, builds the match tree (successor
      generator) and then does a Dijkstra regression search to compute
      all final h-values (stored in distances). operator_costs can
      specify individual operator costs for each operator for action
      cost partitioning. If left empty, default operator costs are used.
    */
    void create_pdb(
        const std::vector<int> &operator_costs = std::vector<int>());

    /*
      Sets the pattern for the PDB and initializes hash_multipliers and
      num_states. operator_costs can specify individual operator costs
      for each operator for action cost partitioning. If left empty,
      default operator costs are used.
    */
    void set_pattern(
        const std::vector<int> &pattern,
        const std::vector<int> &operator_costs = std::vector<int>());

    /*
      For a given abstract state (given as index), the according values
      for each variable in the state are computed and compared with the
      given pairs of goal variables and values. Returns true iff the
      state is a goal state.
    */
    bool is_goal_state(
        const std::size_t state_index,
        const std::vector<std::pair<int, int> > &abstract_goal) const;

    /*
      The given concrete state is used to calculate the index of the
      according abstract state. This is only used for table lookup
      (distances) during search.
    */
    std::size_t hash_index(const GlobalState &state) const;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &state) override;
public:
    /*
      Important: It is assumed that the pattern (passed via Options) is
      sorted, contains no duplicates and is small enough so that the
      number of abstract states is below numeric_limits<int>::max()
      Parameters:
       dump:           If set to true, prints the construction time.
       operator_costs: Can specify individual operator costs for each
       operator. This is useful for action cost partitioning. If left
       empty, default operator costs are used.
    */
    PDBHeuristic(const Options &opts,
                 bool dump = true,
                 const std::vector<int> &operator_costs = std::vector<int>());
    virtual ~PDBHeuristic() override;

    // Returns the pattern (i.e. all variables used) of the PDB
    const std::vector<int> &get_pattern() const {
        return pattern;
    }

    // Returns the size (number of abstrat states) of the PDB
    std::size_t get_size() const {
        return num_states;
    }

    /*
      Returns the average h-value over all states, where dead-ends are
      ignored (they neither increase the sum of all h-values nor the
      number of entries for the mean value calculation). If all states
      are dead-ends, infinity is retuned.
      Note: This is only calculated when called; avoid repeated calls to
      this method!
    */
    double compute_mean_finite_h() const;

    // Returns true iff op has an effect on a variable in the pattern.
    bool is_operator_relevant(const GlobalOperator &op) const;
};

#endif
