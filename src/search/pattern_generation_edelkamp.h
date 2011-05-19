#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

#include "heuristic.h"

#include <ext/hash_map>
#include <map>
#include <vector>

class Options;
class State;
class PDBHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp
class PatternGenerationEdelkamp : public Heuristic {
    const int pdb_max_size; // maximum number of states for each pdb
    const int num_collections;
    const int num_episodes;
    const double mutation_probability;
    const bool disjoint_patterns; // specifies whether patterns in each pattern collection need to be disjoint or not
    const int cost_type;
    std::map<std::vector<bool>, double> pattern_to_fitness; // caches fitness values for already calculated patterns
    std::vector<std::vector<std::vector<bool> > > pattern_collections; // all current pattern collections
    std::vector<PDBHeuristic *> final_pattern_collection; // stores the best pattern collection for fast acces during search
    std::vector<std::vector<int> > operator_costs; // stores operator costs to remember which operators have been used

    // select each pattern collection with a probability (obtained by normalizing the fitness function)
    void select(const std::vector<std::pair<double, int> > &fitness_values, double fitness_sum);

    //void recombine();
    void mutate(); // flip bits (= variables) with a given probability

    // transforms a vector of bools (pattern representation in this algorithm) to the "normal" pattern form
    // vector<int> (needed for PDBHeuristic)
    void transform_to_pattern_normal_form(const std::vector<bool> &bitvector, std::vector<int> &pattern) const;

    /*
    Calculates the mean h-value (fitness function) for each pattern collection and returns the sum of them
    */
    double evaluate(std::vector<std::pair<double, int> > &fitness_values);

    /*
    Calculates the initial pattern collections with a next-fit bin packing algorithm.
    Variables are shuffled to obtain a different random ordering for every pattern collection. Then, variables
    are inserted into a pattern as long as pdb_max_sizes allows. If the "bin" is full, a new one is opened.
    This is repeated until all variables have been treated.
    Every initial pattern collection contains every variable exactly once and all initial patterns of each
    pattern collection is disjoint (regardless of the disjoint_patterns flag).
    */
    void bin_packing();

    /*
    Main genetic algorithm loop. All pattern collections are initialized with bin packing. In each iteration
    (or episode), all patterns are first mutated, then evaluated and finally some patterns in each collection
    are selected to be part of the next episode. Note that we do not do any kind of recombination.
    The best pattern collection (according to their fitness values from the evaluation step) is saved and
    updated during all episodes and stored in the end for the search.
    */
    void genetic_algorithm();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PatternGenerationEdelkamp(const Options &opts);
    virtual ~PatternGenerationEdelkamp();
    void dump() const;
};

#endif
