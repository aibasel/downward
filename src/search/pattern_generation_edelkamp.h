#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

#include "operator_cost.h"

#include <map>
#include <vector>

class Options;
class PDBHeuristic;
class ZeroOnePartitioningPdbCollectionHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp
class PatternGenerationEdelkamp {
    const int pdb_max_size; // maximum number of states for each pdb
    const int num_collections;
    const int num_episodes;
    const double mutation_probability;
    const bool disjoint_patterns; // specifies whether patterns in each pattern collection need to be disjoint or not
    const OperatorCost cost_type;
    std::vector<std::vector<std::vector<bool> > > pattern_collections; // all current pattern collections
    ZeroOnePartitioningPdbCollectionHeuristic *final_heuristic;
    std::vector<int> operator_costs; // stores operator costs to remember which operators have been used

    /* The fitness values (from evaluate) are normalized into probabilities. Then num_collections many
       pattern collections are chosen from the vector of all pattern collections according to their
       probabilities. If all fitness values are 0, we select uniformly randomly. */
    void select(const std::vector<double> &fitness_values);

    /* Iterate over all patterns and flip every variable (set 0 if 1 or 1 if 0) with the given probability
       from options. This method does not check for pdb_max_size or disjoint patterns. */
    void mutate();

    /* Transforms a vector of bools (internal pattern representation in this class) to the "normal" pattern form
       vector<int>, which we need for PDBHeuristic. */
    void transform_to_pattern_normal_form(const std::vector<bool> &bitvector, std::vector<int> &pattern) const;

    /* Update the best pattern collection found so far and its fitness value. */
    void update_best_collection(const std::vector<double> &fitness_values,
                                double &current_best_h,
                                std::vector<std::vector<bool> > &current_best_collection) const;

    /* Calculates the mean h-value (fitness value) for each pattern collection.
       For each pattern collection, we iterate over all patterns, first checking whether they respect the
       size limit, then modifying them in a way that only causally relevant variables remain in the patterns.
       Then the mean h-value for each pattern is calculated (dead ends are ignored) and summed up for the
       entire collection. The total sum of all collections is returned for normalizing purposes in "select". */
    void evaluate(std::vector<double> &fitness_values);
    bool is_pattern_too_large(const std::vector<int> &pattern) const;

    /* Mark used variables in variables_used and return true iff
       anything was already used (in which case we do not mark the
       remaining variables). */
    bool mark_used_variables(const std::vector<int> &pattern,
                             std::vector<bool> &variables_used) const;
    void remove_irrelevant_variables(std::vector<int> &pattern) const;

    /* Calculates the initial pattern collections with a next-fit bin packing algorithm.
       Variables are shuffled to obtain a different random ordering for every pattern collection. Then, variables
       are inserted into a pattern as long as pdb_max_sizes allows. If the "bin" is full, a new one is opened.
       This is repeated until all variables have been treated.
       Every initial pattern collection contains every variable exactly once and all initial patterns of each
       pattern collection are disjoint (regardless of the disjoint_patterns flag). */
    void bin_packing();

    /* Main genetic algorithm loop. All pattern collections are initialized with bin packing. In each iteration
       (or episode), all patterns are first mutated, then evaluated and finally some patterns in each collection
       are selected to be part of the next episode. Note that we do not do any kind of recombination.
       The best pattern collection (according to their fitness values from the evaluation step) is saved and
       updated during all episodes and stored in the end for the search. */
    void genetic_algorithm();
public:
    PatternGenerationEdelkamp(const Options &opts);
    virtual ~PatternGenerationEdelkamp();

    /* Returns the ZeroOnePartitioningPdbCollectionHeuristic created by PatternGenerationEdelkamp.
       Important: caller owns the returned pointer and has to take care of its deletion. */
    ZeroOnePartitioningPdbCollectionHeuristic *get_pattern_collection_heuristic() const { return final_heuristic; }
    void dump() const;
};

#endif
