#ifndef PDBS_PDB_HEURISTIC_H
#define PDBS_PDB_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

class AbstractOperator {
    /*
    This class represents an abstract operator how it is needed for the regression search performed during the
    PDB-construction. As all abstract states are represented as a number, abstract operators don't have "usual"
    effects but "hash effects", i.e. the change (as number) the abstract operator implies on a given abstract state.
    */

    int cost;

    // Preconditions for the regression search, corresponds to normal effects and prevail of concrete operators
    std::vector<std::pair<int, int> > regression_preconditions;

    // Effect of the operator during regression search on a given abstract state number
    size_t hash_effect;
public:
    /* Abstract operators are built from concrete operators. The parameters follow the usual name convention of SAS+
       operators, meaning prevail, preconditions and effects are all related to progression search. */
    AbstractOperator(const std::vector<std::pair<int, int> > &prevail,
                     const std::vector<std::pair<int, int> > &preconditions,
                     const std::vector<std::pair<int, int> > &effects, int cost,
                     const std::vector<size_t> &hash_multipliers);
    ~AbstractOperator();

    // Returns variable value pairs which represent the preconditions of the abstract operator in a regression search
    const std::vector<std::pair<int, int> > &get_regression_preconditions() const {return regression_preconditions; }

    // Returns the effect of the abstract operator in form of a value change (+ or -) to an abstract state index
    size_t get_hash_effect() const {return hash_effect; }

    // Returns the cost of the abstract operator (same as the cost of the original concrete operator)
    int get_cost() const {return cost; }
    void dump(const std::vector<int> &pattern) const;
};

// Implements a single PDB
class Operator;
class State;
class PDBHeuristic : public Heuristic {
    std::vector<int> pattern;

    // can be specified to be different from the normal operator costs. this is useful for action cost partitioning
    std::vector<int> operator_costs;

    std::vector<bool> relevant_operators; // stores for all operators whether they are relevant to this PDB or not
    size_t num_states; // size of the PDB

    // concrete variable are mapped to abstract variables in the order they appear in pattern
    std::vector<int> variable_to_index;

    // final h-values for abstract-states. dead-ends are represented by numeric_limits<int>::max()
    std::vector<int> distances;

    std::vector<size_t> hash_multipliers; // multipliers for each variable for perfect hash function

    /* Recursive method; called by build_abstract_operators.
       In the case of a precondition with value = -1 in the conrete operator, all mutliplied out abstract
       operators are computed, i.e. for all possible values of the variable (with precondition = -1),
       one abstract operator with a conrete value (!= -1) is computed. */
    void multiply_out(int pos, int op_no, int cost, std::vector<std::pair<int, int> > &prev_pairs,
                      std::vector<std::pair<int, int> > &pre_pairs,
                      std::vector<std::pair<int, int> > &eff_pairs,
                      const std::vector<std::pair<int, int> > &effects_without_pre,
                      std::vector<AbstractOperator> &operators);

    /* Computes all abstract operators for a given concrete operator (by its global operator number). Initializes
       datastructures for initial call to recursive method multiyply_out. */
    void build_abstract_operators(int op_no, std::vector<AbstractOperator> &operators);

    /* Computes all abstract operators, builds the match tree (successor generator) and then does a Dijkstra regression
       search to compute all final h-values (stored in distances). */
    void create_pdb();

    // Sets the pattern for the PDB and initializes hash_multipliers and num_states.
    void set_pattern(const std::vector<int> &pattern);

    /* For a given abstract state (given as index), the according values for each variable in the state are computed
       and compared with the given pairs of goal variables and values. Returns true iff the state is a goal state. */
    bool is_goal_state(const size_t state_index, const std::vector<std::pair<int, int> > &abstract_goal) const;

    /* The given concrete state is used to calculate the index of the according abstract state. This is only used
       for table lookup (distances) during search. */
    size_t hash_index(const State &state) const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    /* Important: It is assumed that the pattern (passed via Options) is small enough so that the number of
                  abstract states is below numeric_limits<int>::max()
       Parameters:
       dump:     If set to true, prints the construction time.
       op_costs: Can specify individual operator costs for each operator. This is useful for action cost
                 partitioning. If left empty, default operator costs are used. */
    PDBHeuristic(const Options &opts,
                 bool dump = true,
                 const std::vector<int> &op_costs = std::vector<int>());
    virtual ~PDBHeuristic();

    // Returns the pattern (i.e. all variables used) of the PDB
    const std::vector<int> &get_pattern() const {return pattern; }

    // Returns the size (number of abstrat states) of the PDB
    size_t get_size() const {return num_states; }

    /* Returns the average h-value over all states, where dead-ends are ignored (they neither increase
       the sum of all h-values nor the total number of entries for the mean value calculation). If pattern
       is empty or all states are dead-ends, infinity is retuned.
       Note: This is only calculated when called; avoid repeated calls to this method! */
    double compute_mean_finite_h() const;

    // Returns all operators affecting this PDB
    const std::vector<bool> &get_relevant_operators() const {return relevant_operators; }
};

#endif
