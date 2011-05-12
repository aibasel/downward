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
    // AbstractOperators are built from normal operators; the parameters follow the usual name convetion of SAS+
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
    std::vector<int> operator_costs;
    std::vector<bool> used_operators;
    size_t num_states;
    std::vector<int> variable_to_index;
    std::vector<int> distances; // final h-values for abstract-states
    std::vector<size_t> hash_multipliers; // multipliers for each variable for perfect hash function
    void verify_no_axioms_no_cond_effects() const; // SAS+ tasks only

    // build_abstract_operators computes all abstract operators for each normal operator
    // in the case of pre = -1, for each possible value of the concerned variable, an abstract
    // operator is computed. calls the recursive method "build_recursively"
    void build_recursively(int pos, int op_no, int cost,  std::vector<std::pair<int, int> > &prev_pairs,
                           std::vector<std::pair<int, int> > &pre_pairs,
                           std::vector<std::pair<int, int> > &eff_pairs,
                           const std::vector<std::pair<int, int> > &effects_without_pre,
                           std::vector<AbstractOperator> &operators);
    void build_abstract_operators(int op_no, std::vector<AbstractOperator> &operators);
    
    void create_pdb(); // builds the graph-structure and does a Dijkstra-backward-search
    void set_pattern(const std::vector<int> &pattern); // initializes hash_multipliers and num_states
    bool is_goal_state(const size_t state_index, const std::vector<std::pair<int, int> > &abstract_goal) const;
    size_t hash_index(const State &state) const; // maps a state to an index
    //AbstractState inv_hash_index(const size_t index) const; // inverts the hash-index-function (returns an abstract state)
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PDBHeuristic(const Options &opts,
                 bool dump=true,
                 const std::vector<int> &op_costs=std::vector<int>());
    virtual ~PDBHeuristic();
    const std::vector<int> &get_pattern() const { return pattern; }
    const std::vector<int> &get_h_values() const { return distances; }
    const std::vector<bool> &get_used_ops() const { return used_operators; }
    void dump() const;
};

#endif
