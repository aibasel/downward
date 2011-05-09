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
    int pdb_max_size;
    int num_collections;
    int num_episodes;
    double mutation_probability;
    bool disjoint_patterns;
    int cost_type;
    //__gnu_cxx::hash_map<std::vector<bool>, double> pattern_to_fitness;
    std::map<std::vector<bool>, double> pattern_to_fitness; // cache fitness values for already calculated patterns
    std::vector<std::vector<std::vector<bool> > > pattern_collections; // all current pattern collections
    std::vector<PDBHeuristic *> final_pattern_collection;
    std::vector<std::vector<int> > operator_costs; // stores operator costs to remember which operators have been used

    // select each pattern collection with a probability (obtained by normalizing the fitness function)
    void select(const std::vector<std::pair<double, int> > &fitness_values, double fitness_sum);

    //void recombine();
    void mutate(); // flip bits (= variables) with a given probability

    // transforms a vector of bools (pattern representation in this algorithm) to the "normal" pattern form
    // vector<int> (needed for PDBHeuristic)
    void transform_to_pattern_normal_form(const std::vector<bool> &bitvector, std::vector<int> &pattern) const;

    // calculate the mean h-value (fitness function) for each pattern collection and returns the sum of them
    double evaluate(std::vector<std::pair<double, int> > &fitness_values);

    void bin_packing(); // bin packing (for variables) to determine initial pattern collections
    void genetic_algorithm(); // main genectic algorithm loop
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PatternGenerationEdelkamp(const Options &opts);
    virtual ~PatternGenerationEdelkamp();
    //PDBCollectionHeuristic *get_pattern_collection_heuristic() const;
    void dump() const;
};

#endif
