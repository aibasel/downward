#ifndef PATTERN_GENERATION_HASLUM_H
#define PATTERN_GENERATION_HASLUM_H

#include "operator_cost.h"

#include <map>
#include <vector>

class Options;
class PDBCollectionHeuristic;
class PDBHeuristic;
class State;
// Implementation of the pattern generation algorithm by Haslum et al.
class PatternGenerationHaslum {
    const int pdb_max_size; // maximum number of states for each pdb
    const int collection_max_size; // maximum added size of all pdbs
    const int num_samples;
    const int min_improvement; // minimal improvement required for hill climbing to continue search
    const OperatorCost cost_type;
    PDBCollectionHeuristic *current_heuristic;

    /*
    For the given pattern, all possible extensions of the pattern by one relevant variable
    are inserted into candidate_patterns (not necessarily empty, as candidate patterns always remain
    candiates in future iterations, because pattern does not get deleted from the current collection).
    TODO: can this method really produce duplicates from ONE pattern? i doubt it. it may rather
    happen that the candidate already exists in candidate_patterns...
    */
    void generate_candidate_patterns(const std::vector<int> &pattern,
                                     std::vector<std::vector<int> > &candidate_patterns);

    /*
    Performs num_samples random walks with a lenght (different for each random walk) chosen
    according to a binomial distribution with n = 4 * solution depth estimate and p = 0.5,
    starting from the initial state. In each step of a random walk, a random operator is taken
    and applied to the current state. If a dead end is reached or no more operators are
    applicable, the walk starts over again from the initial state. At the end of each random
    walk, the last state visited is taken as a sample state, thus totalling exactly
    num_samples of sample states.
    */
    void sample_states(std::vector<State> &samples, double average_operator_costs);

    /*
    Returns true iff the h-value of the new pattern (from pdb_heuristic) plus the h-value of all
    additive subsets from the current pattern collection heuristic if the new pattern was added
    to it is greater than the the h-value of the current pattern collection.
    */
    bool is_heuristic_improved(PDBHeuristic *pdb_heuristic, const State &sample);

    /*
    This is the core algorithm of this class. As soon as after an iteration, the improvement (according
    to the "counting approximation") is smaller than the minimal required improvement, the search is
    stopped. This method uses a map to store PDBs to avoid recomputation of the same PDBs later. This
    is quite a large time gain, but may use too much memory.
    */
    void hill_climbing(int average_operator_costs, std::vector<std::vector<int> > &candidate_patterns);

    /*
    Initializes everything for the hill climbing algorithm. Note that the initial pattern collection
    (consisting of exactly one PDB for each goal variable) may break the maximum collection size limit,
    if the latter is set too small or if there are many goal variables with a large domain.
    */
    void initialize();
public:
    PatternGenerationHaslum(const Options &opts);
    virtual ~PatternGenerationHaslum();

    /*
    Returns the PDBCollectionHeuristic created by PatternGenerationHaslum
    Important: caller owns the returned pointer and has to take care of its deletion
    */
    PDBCollectionHeuristic *get_pattern_collection_heuristic() const { return current_heuristic; }
};

#endif
