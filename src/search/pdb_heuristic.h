#ifndef PDB_HEURISTIC_H
#define PDB_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class AbstractOperator {
    /*
    This class represents an abstract operator how it is needed for the regression search performed during the
    PDB-construction. As all astract states are represented as a number, abstract operators don't have "usual"
    effects but "hash effects", i.e. the change (as number) the abstract operator implies on a given abstract state.
    */
    int cost;

    // preconditions for the regression search, corresponds to normal effects and prevail of concrete operators
    std::vector<std::pair<int, int> > regression_preconditions;

    size_t hash_effect; // effect of the operator during regression search on a given abstract state number
public:
    // abstract operators are built from concrete operators. the parameters follow the usual name convetion of SAS+
    // operators, meaning prevail, preconditions and effects are all related to progression search
    AbstractOperator(const std::vector<std::pair<int, int> > &prevail,
                     const std::vector<std::pair<int, int> > &preconditions,
                     const std::vector<std::pair<int, int> > &effects, int cost,
                     const std::vector<size_t> &hash_multipliers);
    ~AbstractOperator();
    const std::vector<std::pair<int, int> > &get_regression_preconditions() const { return regression_preconditions; }
    size_t get_hash_effect() const { return hash_effect; }
    int get_cost() const { return cost; }
    void dump(const std::vector<int> &pattern) const;
};

// Implements a single PDB
class Operator;
class State;
class PDBHeuristic : public Heuristic {
    std::vector<int> pattern;

    // can be specified to be different from the normal operator costs. this is useful for action cost partitioning
    std::vector<int> operator_costs;

    std::vector<bool> relevant_operators; // stores for all operators wheather they are used in this PDB or not
    size_t num_states;

    // concrete variable are mapped to abstract variables in the order they appear in pattern
    std::vector<int> variable_to_index;

    // final h-values for abstract-states. dead-ends are represented by numeric_limits<int>::max()
    std::vector<int> distances;

    std::vector<size_t> hash_multipliers; // multipliers for each variable for perfect hash function
    void verify_no_axioms_no_cond_effects() const; // we support SAS+ tasks only

    // recursive method; called by build_abstract_operators
    void multiply_out(int pos, int op_no, int cost,  std::vector<std::pair<int, int> > &prev_pairs,
                      std::vector<std::pair<int, int> > &pre_pairs,
                      std::vector<std::pair<int, int> > &eff_pairs,
                      const std::vector<std::pair<int, int> > &effects_without_pre,
                      std::vector<AbstractOperator> &operators);

    // computes all abstract operators for a given concrete operator. in the case of a precondition with value = -1
    // in the conrete operator, all mutliplied out abstract operators are computed, i.e. for all possible values
    // of the variable (with precondition = -1), one abstract operator is computed.
    void build_abstract_operators(int op_no, std::vector<AbstractOperator> &operators);

    // compute all abstract operators, builds the match tree (successor generator) and then does a Dijkstra regression
    // search to compute all final h-values (stored in distances)
    void create_pdb();

    void set_pattern(const std::vector<int> &pattern); // initializes hash_multipliers and num_states
    bool is_goal_state(const size_t state_index, const std::vector<std::pair<int, int> > &abstract_goal) const;

    // maps a state to an index (representing the abstract state). used for table lookup (distances) during search
    size_t hash_index(const State &state) const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic(const Options &opts,
                 bool dump=true, // prints construction time when set to true
                 const std::vector<int> &op_costs=std::vector<int>()); // if nothing specified, use default operator costs
    virtual ~PDBHeuristic();

    // returns the variables of the PDB
    const std::vector<int> &get_pattern() const { return pattern; }

    // returns h-values (the actual PDB) for all abstract states. dead-ends are represented by numeric_limits<int>::max()
    const std::vector<int> &get_h_values() const { return distances; }

    // returns all operators affecting this PDB
    const std::vector<bool> &get_relevant_operators() const { return relevant_operators; }
    void dump() const;
};

#endif
